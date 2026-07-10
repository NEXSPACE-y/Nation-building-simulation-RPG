using System.Globalization;
using System.Text;
using System.Text.Json;

namespace NationSimulation.StageB;

public sealed class Simulation
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower,
        PropertyNameCaseInsensitive = true,
        WriteIndented = false
    };

    private readonly string _fixturePath;
    private readonly string _fixtureId;
    private readonly string _schemaVersion;
    private readonly string _simulationVersion;
    private readonly int _realSecondsPerGameHour;
    private readonly long _offlineLimitSeconds;
    private readonly double _witnessThreshold;
    private readonly HashSet<string> _allowedActions;
    private readonly Dictionary<string, double> _locationVisibility;
    private readonly Dictionary<string, int> _rootProcessedCounts = [];
    private ulong _nextEventSequence = 1;

    private Simulation(string fixturePath, FixtureDocument fixture)
    {
        _fixturePath = fixturePath;
        _fixtureId = fixture.FixtureId;
        _schemaVersion = fixture.SchemaVersion;
        _simulationVersion = fixture.SimulationVersion;
        WorldSeed = fixture.World.WorldSeed;
        CurrentWorldTimeMinutes = fixture.World.CurrentWorldTimeMinutes;
        LastSimulatedAt = fixture.World.LastSimulatedAt;
        _realSecondsPerGameHour = fixture.Settings.RealSecondsPerGameHour;
        _offlineLimitSeconds = fixture.Settings.OfflineLimitSeconds;
        ChainLimit = fixture.Settings.ChainLimit;
        _witnessThreshold = fixture.Settings.WitnessThreshold;
        _allowedActions = new HashSet<string>(fixture.AllowedPlayerActions, StringComparer.Ordinal);
        _locationVisibility = fixture.Locations.ToDictionary(
            location => location.LocationId,
            location => Math.Clamp(location.Visibility * (1.0 - location.Obstruction), 0.0, 1.0),
            StringComparer.Ordinal);
        Country = fixture.Country;
        Player = fixture.Player;
        AiNpcs = fixture.AiNpcs;
        NonAiNpcs = fixture.NonAiNpcs;
    }

    public ulong WorldSeed { get; private set; }
    public long CurrentWorldTimeMinutes { get; private set; }
    public long LastSimulatedAt { get; private set; }
    public long SimulationTick { get; private set; }
    public int ChainLimit { get; private set; }
    public CountryState Country { get; private set; }
    public PlayerState Player { get; private set; }
    public List<AiNpcState> AiNpcs { get; }
    public List<NonAiNpcState> NonAiNpcs { get; }
    public List<SimEvent> EventHistory { get; } = [];
    public List<AuditEntry> AuditLog { get; } = [];
    public Queue<SimEvent> PendingEvents { get; } = [];

    public static Simulation FromFixture(string fixturePath)
    {
        var json = File.ReadAllText(fixturePath, Encoding.UTF8);
        var fixture = JsonSerializer.Deserialize<FixtureDocument>(json, JsonOptions)
            ?? throw new InvalidDataException("Fixture JSON deserialized to null.");
        ValidateFixture(fixture);
        return new Simulation(Path.GetFullPath(fixturePath), fixture);
    }

    private static void ValidateFixture(FixtureDocument fixture)
    {
        if (!fixture.FixtureOnly)
            throw new InvalidDataException("Stage B requires fixture_only=true.");
        if (fixture.AiNpcs.Count != 20 || fixture.NonAiNpcs.Count != 20)
            throw new InvalidDataException("Stage B requires 20 AI NPCs and 20 NON AI NPCs.");
        if (fixture.Locations.Count != 5)
            throw new InvalidDataException("Stage B requires exactly five fixture locations.");
        if (fixture.Settings.RealSecondsPerGameHour <= 0 || fixture.Settings.OfflineLimitSeconds < 0 ||
            fixture.Settings.ChainLimit <= 0)
            throw new InvalidDataException("Invalid timing or chain settings.");

        var ids = new HashSet<string>(StringComparer.Ordinal);
        foreach (var npc in fixture.AiNpcs)
        {
            if (!ids.Add(npc.NpcId)) throw new InvalidDataException($"Duplicate AI NPC id: {npc.NpcId}");
            if (npc.StateSlotCount != 255 || npc.States.Count != 255)
                throw new InvalidDataException($"{npc.NpcId} must define exactly 255 state slots.");
            for (var index = 0; index < npc.States.Count; index++)
            {
                var state = npc.States[index];
                if (state.StateId != index + 1)
                    throw new InvalidDataException($"{npc.NpcId} state ids must be ordered 1..255.");
                if (state.Undefined && state.StateName != "UNDEFINED")
                    throw new InvalidDataException($"{npc.NpcId} has a non-explicit undefined state.");
            }
            if (npc.CurrentStateId is < 1 or > 255 || npc.States[npc.CurrentStateId - 1].Undefined)
                throw new InvalidDataException($"{npc.NpcId} has an invalid initial state.");
        }
        if (fixture.NonAiNpcs.Select(npc => npc.NpcId).Distinct(StringComparer.Ordinal).Count() != 20)
            throw new InvalidDataException("Duplicate NON AI NPC ids.");
    }

    public AiNpcState AiNpc(string npcId) =>
        AiNpcs.FirstOrDefault(npc => npc.NpcId == npcId)
        ?? throw new KeyNotFoundException($"Unknown AI NPC: {npcId}");

    public NonAiNpcState NonAiNpc(string npcId) =>
        NonAiNpcs.FirstOrDefault(npc => npc.NpcId == npcId)
        ?? throw new KeyNotFoundException($"Unknown NON AI NPC: {npcId}");

    public SimEvent Event(string eventId) =>
        EventHistory.FirstOrDefault(value => value.EventId == eventId)
        ?? throw new KeyNotFoundException($"Unknown processed event: {eventId}");

    public string EnqueuePlayerAction(string action, string targetId = "", string destinationId = "")
    {
        if (!_allowedActions.Contains(action))
            throw new ArgumentException($"Action is not enabled by fixture: {action}", nameof(action));

        var targetIsAi = false;
        var targetType = "";
        if (action is "HELP" or "HARM" or "TALK" or "TRADE" or "STEAL")
        {
            if (string.IsNullOrEmpty(targetId)) throw new ArgumentException($"{action} requires a target id.");
            var ai = AiNpcs.FirstOrDefault(npc => npc.NpcId == targetId);
            var nonAi = NonAiNpcs.FirstOrDefault(npc => npc.NpcId == targetId);
            if (ai is null && nonAi is null) throw new ArgumentException($"Unknown action target: {targetId}");
            targetIsAi = ai is not null;
            targetType = targetIsAi ? "AI_NPC" : "NON_AI_NPC";
            var targetLocation = ai?.CurrentLocationId ?? nonAi!.CurrentLocationId;
            if (targetLocation != Player.CurrentLocationId)
                throw new ArgumentException("Player and target must be in the same location.");
        }
        if (action == "MOVE")
        {
            if (!_locationVisibility.ContainsKey(destinationId))
                throw new ArgumentException($"Unknown MOVE destination: {destinationId}");
            if (destinationId == Player.CurrentLocationId)
                throw new ArgumentException("MOVE destination must differ from the current location.");
        }

        var simEvent = MakeEvent(ActionEventType(action, targetIsAi), Player.PlayerId, "PLAYER",
            targetId, targetType, Player.CurrentLocationId);
        simEvent.Payload["player_action"] = action;
        simEvent.Payload["salience"] = EventSalience(simEvent.EventType).ToString("R", CultureInfo.InvariantCulture);
        if (action == "MOVE")
        {
            simEvent.Payload["previous_location_id"] = Player.CurrentLocationId;
            simEvent.Payload["destination_id"] = destinationId;
            Player.CurrentLocationId = destinationId;
        }
        else if (action == "WAIT")
        {
            simEvent.Payload["game_minutes"] = "60";
            simEvent.Payload["offline"] = "false";
        }
        simEvent.RootEventId = simEvent.EventId;
        Player.ActionHistory.Add($"{action}:{(string.IsNullOrEmpty(targetId) ? destinationId : targetId)}");
        Player.UpdatedAt = CurrentWorldTimeMinutes;
        PendingEvents.Enqueue(simEvent);
        return simEvent.EventId;
    }

    public string PlayerAction(string action, string targetId = "", string destinationId = "")
    {
        var id = EnqueuePlayerAction(action, targetId, destinationId);
        ProcessPending();
        return id;
    }

    public string EnqueueRumor(string nonAiNpcId, string targetAiNpcId, string originalEventId, double credibility)
    {
        if (credibility is < 0.0 or > 1.0) throw new ArgumentOutOfRangeException(nameof(credibility));
        var source = NonAiNpc(nonAiNpcId);
        var target = AiNpc(targetAiNpcId);
        var original = Event(originalEventId);
        if (!source.TemporaryMemory.Contains(originalEventId))
            throw new InvalidOperationException("NON AI NPC does not remember the source event.");

        var rumor = MakeEvent("RUMOR_CREATED", nonAiNpcId, "NON_AI_NPC", targetAiNpcId, "AI_NPC",
            source.CurrentLocationId);
        rumor.Credibility = credibility;
        rumor.EvidenceLevel = original.EvidenceLevel * credibility;
        rumor.SourceEventId = original.EventId;
        rumor.RootEventId = original.RootEventId;
        rumor.Payload["original_event_id"] = original.EventId;
        rumor.Payload["subject_event_type"] = original.EventType;
        rumor.Payload["original_actor"] = original.ActorId;
        rumor.Payload["original_target"] = original.TargetId;
        rumor.Payload["original_location"] = original.LocationId;
        rumor.Payload["distortion_level"] = (1.0 - credibility).ToString("R", CultureInfo.InvariantCulture);
        rumor.Payload["hop_count"] = "0";
        rumor.Payload["target_location"] = target.CurrentLocationId;
        PendingEvents.Enqueue(rumor);
        return rumor.EventId;
    }

    public string ShareRumor(string nonAiNpcId, string targetAiNpcId, string originalEventId, double credibility)
    {
        var id = EnqueueRumor(nonAiNpcId, targetAiNpcId, originalEventId, credibility);
        ProcessPending();
        return id;
    }

    public int ProcessPending(int maxEvents = int.MaxValue)
    {
        var processed = 0;
        var lastActor = "";
        while (PendingEvents.Count > 0 && processed < maxEvents)
        {
            var current = PendingEvents.Dequeue();
            var root = string.IsNullOrEmpty(current.RootEventId) ? current.EventId : current.RootEventId;
            _rootProcessedCounts.TryGetValue(root, out var rootCount);
            if (rootCount >= ChainLimit)
            {
                var retained = new Queue<SimEvent>();
                var removed = 1;
                while (PendingEvents.Count > 0)
                {
                    var pending = PendingEvents.Dequeue();
                    if (pending.RootEventId == root) removed++;
                    else retained.Enqueue(pending);
                }
                while (retained.Count > 0) PendingEvents.Enqueue(retained.Dequeue());
                AppendChainLimitEvent(root, removed, string.IsNullOrEmpty(lastActor) ? current.ActorId : lastActor);
                processed++;
                continue;
            }
            _rootProcessedCounts[root] = rootCount + 1;
            processed++;
            lastActor = current.ActorId;
            var children = DeriveEvents(current);
            foreach (var child in children)
            {
                if (string.IsNullOrEmpty(child.SourceEventId)) child.SourceEventId = current.EventId;
                if (string.IsNullOrEmpty(child.RootEventId)) child.RootEventId = root;
                current.ResultingEvents.Add(child.EventId);
            }
            AppendHistory(current);
            foreach (var child in children) PendingEvents.Enqueue(child);
        }
        return processed;
    }

    private List<SimEvent> DeriveEvents(SimEvent simEvent)
    {
        if (simEvent.EventType is "RUMOR_CREATED" or "RUMOR_TRANSFERRED") return DeriveRumorEvent(simEvent);
        if (simEvent.EventType is "AI_NPC_OBSERVED_EVENT" or "AI_NPC_HEARD_EVENT") return DerivePerceptionEvent(simEvent);
        if (simEvent.ActorType == "PLAYER" || simEvent.EventType == "TIME_ELAPSED") return DerivePlayerEvent(simEvent);
        return [];
    }

    private List<SimEvent> DerivePlayerEvent(SimEvent simEvent)
    {
        List<SimEvent> generated = [];
        if (simEvent.EventType == "PLAYER_LEFT_LOCATION")
        {
            var entered = MakeEvent("PLAYER_ENTERED_LOCATION", Player.PlayerId, "PLAYER", "", "",
                simEvent.Payload["destination_id"]);
            entered.Payload["previous_location_id"] = simEvent.Payload["previous_location_id"];
            generated.Add(entered);
            return generated;
        }
        if (simEvent.EventType == "TIME_ELAPSED")
        {
            var minutes = long.Parse(simEvent.Payload["game_minutes"], CultureInfo.InvariantCulture);
            CurrentWorldTimeMinutes += minutes;
            if (simEvent.Payload["offline"] == "true")
            {
                var completed = MakeEvent("OFFLINE_SIMULATION_COMPLETED", "world_001", "WORLD", "", "",
                    Player.CurrentLocationId);
                completed.Payload = new Dictionary<string, string>(simEvent.Payload, StringComparer.Ordinal)
                {
                    ["elapsed_game_minutes"] = minutes.ToString(CultureInfo.InvariantCulture)
                };
                generated.Add(completed);
            }
            return generated;
        }

        if (simEvent.TargetType == "NON_AI_NPC" && simEvent.EventType is
            "PLAYER_HELPED_NON_AI_NPC" or "PLAYER_HARMED_NON_AI_NPC" or "PLAYER_TRADED" or "PLAYER_STOLE" or "PLAYER_TALKED")
            NonAiNpc(simEvent.TargetId).TemporaryMemory.Add(simEvent.EventId);

        switch (simEvent.EventType)
        {
            case "PLAYER_HELPED_NON_AI_NPC":
                Player.Reputation++;
                Player.AchievementTags.Add("HELPED_CIVILIAN");
                break;
            case "PLAYER_HARMED_NON_AI_NPC": Player.AchievementTags.Add("ASSAULTED_CIVILIAN"); break;
            case "PLAYER_STOLE": Player.AchievementTags.Add("STOLE_FROM_CIVILIAN"); break;
            case "PLAYER_TRADED": Player.AchievementTags.Add("SUCCESSFUL_TRADE"); break;
        }

        if (simEvent.TargetType == "AI_NPC" && simEvent.EventType is "PLAYER_HELPED_AI_NPC" or "PLAYER_HARMED_AI_NPC")
        {
            simEvent.ObservedBy.Add(simEvent.TargetId);
            var direct = MakeEvent("AI_NPC_OBSERVED_EVENT", simEvent.TargetId, "AI_NPC", simEvent.TargetId,
                "AI_NPC", simEvent.LocationId);
            direct.Payload["subject_event_id"] = simEvent.EventId;
            direct.Payload["subject_event_type"] = simEvent.EventType;
            direct.Payload["perception"] = "OBSERVED";
            direct.Payload["witness_score"] = "1.0";
            generated.Add(direct);
        }

        var salience = double.Parse(simEvent.Payload.GetValueOrDefault("salience", "0.4"), CultureInfo.InvariantCulture);
        var visibility = _locationVisibility.GetValueOrDefault(simEvent.LocationId, 0.0);
        foreach (var npc in AiNpcs)
        {
            if (npc.CurrentLocationId != simEvent.LocationId || npc.NpcId == simEvent.TargetId) continue;
            var witnessScore = npc.Attention * visibility * salience;
            if (witnessScore < _witnessThreshold) continue;
            simEvent.ObservedBy.Add(npc.NpcId);
            var observed = MakeEvent("AI_NPC_OBSERVED_EVENT", npc.NpcId, "AI_NPC", npc.NpcId, "AI_NPC",
                simEvent.LocationId);
            observed.Payload["subject_event_id"] = simEvent.EventId;
            observed.Payload["subject_event_type"] = simEvent.EventType;
            observed.Payload["perception"] = "OBSERVED";
            observed.Payload["witness_score"] = witnessScore.ToString("R", CultureInfo.InvariantCulture);
            generated.Add(observed);
        }
        return generated;
    }

    private List<SimEvent> DeriveRumorEvent(SimEvent simEvent)
    {
        if (simEvent.EventType == "RUMOR_CREATED")
        {
            var transferred = MakeEvent("RUMOR_TRANSFERRED", simEvent.ActorId, "NON_AI_NPC",
                simEvent.TargetId, "AI_NPC", simEvent.LocationId);
            transferred.Credibility = simEvent.Credibility;
            transferred.EvidenceLevel = simEvent.EvidenceLevel;
            transferred.Payload = new Dictionary<string, string>(simEvent.Payload, StringComparer.Ordinal)
            {
                ["hop_count"] = "1"
            };
            return [transferred];
        }
        simEvent.HeardBy.Add(simEvent.TargetId);
        var heard = MakeEvent("AI_NPC_HEARD_EVENT", simEvent.ActorId, simEvent.ActorType,
            simEvent.TargetId, "AI_NPC", AiNpc(simEvent.TargetId).CurrentLocationId);
        heard.Credibility = simEvent.Credibility;
        heard.EvidenceLevel = simEvent.EvidenceLevel;
        heard.Payload = new Dictionary<string, string>(simEvent.Payload, StringComparer.Ordinal)
        {
            ["perception"] = "HEARD"
        };
        return [heard];
    }

    private List<SimEvent> DerivePerceptionEvent(SimEvent simEvent)
    {
        var npc = AiNpc(simEvent.TargetId);
        simEvent.ProcessedBy.Add(npc.NpcId);
        npc.KnownEvents.Add(simEvent.EventId);
        var audit = new AuditEntry
        {
            Timestamp = CurrentWorldTimeMinutes,
            SimulationTick = SimulationTick + 1,
            EventId = simEvent.EventId,
            RootEventId = simEvent.RootEventId,
            EventActorId = simEvent.ActorId,
            DecisionNpcId = npc.NpcId,
            TargetId = simEvent.TargetId,
            CurrentState = npc.CurrentStateId
        };
        var generated = ExecuteAiReaction(npc, simEvent, audit);
        AuditLog.Add(audit);
        return generated;
    }

    private List<TransitionRule> MatchingRules(AiNpcState npc, SimEvent perception)
    {
        var state = StateDefinition(npc, npc.CurrentStateId);
        if (state.Undefined) throw new InvalidOperationException($"Attempted to use UNDEFINED state for {npc.NpcId}.");
        var subject = perception.Payload.GetValueOrDefault("subject_event_type", "");
        var perceptionKind = perception.Payload.GetValueOrDefault("perception", "");
        return state.TransitionRules.Where(rule =>
        {
            if (rule.TriggerEventType != perception.EventType || rule.SubjectEventType != subject ||
                rule.Perception != perceptionKind || perception.Credibility < rule.MinCredibility ||
                perception.Credibility > rule.MaxCredibility) return false;
            if (rule.OnceOnly && npc.UsedRules.Contains(rule.RuleId)) return false;
            return !npc.RuleLastUsedTick.TryGetValue(rule.RuleId, out var last) ||
                   SimulationTick - last >= rule.Cooldown;
        }).ToList();
    }

    private List<SimEvent> ExecuteAiReaction(AiNpcState npc, SimEvent perception, AuditEntry audit)
    {
        var matches = MatchingRules(npc, perception);
        audit.MatchedRules.AddRange(matches.Select(rule => rule.RuleId));
        if (matches.Count == 0)
        {
            audit.NextState = npc.CurrentStateId;
            return [];
        }
        var highestPriority = matches.Max(rule => rule.Priority);
        var finalists = matches.Where(rule => rule.Priority == highestPriority).ToList();
        var seedMaterial = $"{WorldSeed}|{npc.NpcId}|{perception.EventId}|{npc.CurrentStateId}|{SimulationTick + 1}";
        audit.RandomSeed = DeterministicHash(seedMaterial);
        var selected = finalists[(int)(audit.RandomSeed % (ulong)finalists.Count)];
        audit.SelectedRule = selected.RuleId;
        npc.UsedRules.Add(selected.RuleId);
        npc.RuleLastUsedTick[selected.RuleId] = SimulationTick + 1;

        var oldState = npc.CurrentStateId;
        var nextState = StateDefinition(npc, selected.TargetStateId);
        if (nextState.Undefined) throw new InvalidOperationException($"Transition selected UNDEFINED state for {npc.NpcId}.");
        npc.CurrentStateId = selected.TargetStateId;
        npc.PlayerEvaluation += selected.PlayerEvaluationDelta;
        if (!string.IsNullOrEmpty(nextState.GoalModifier)) npc.CurrentGoal = nextState.GoalModifier;
        npc.UpdatedAt = CurrentWorldTimeMinutes;
        audit.NextState = npc.CurrentStateId;

        var dialogue = nextState.DialogueCandidates
            .Where(candidate => !(candidate.OnceOnly && npc.UsedDialogues.Contains(candidate.DialogueId)))
            .Where(candidate => !npc.DialogueLastUsedTick.TryGetValue(candidate.DialogueId, out var last) ||
                                SimulationTick + 1 - last >= candidate.Cooldown)
            .OrderByDescending(candidate => candidate.Priority).FirstOrDefault();
        if (dialogue is not null)
        {
            audit.SelectedDialogue = dialogue.DialogueId;
            npc.UsedDialogues.Add(dialogue.DialogueId);
            npc.DialogueLastUsedTick[dialogue.DialogueId] = SimulationTick + 1;
        }
        var action = nextState.ActionCandidates.OrderByDescending(candidate => candidate.Priority).FirstOrDefault();
        if (action is not null) audit.SelectedAction = action.ActionType;

        List<SimEvent> generated = [];
        if (selected.RelationshipDelta != 0 && !string.IsNullOrEmpty(selected.RelationshipMetric))
        {
            var relationshipTarget = selected.RelationshipTarget == "EVENT_ACTOR"
                ? perception.ActorId : selected.RelationshipTarget;
            if (!string.IsNullOrEmpty(relationshipTarget) && relationshipTarget != npc.NpcId)
            {
                if (!npc.Relationships.TryGetValue(relationshipTarget, out var metrics))
                    npc.Relationships[relationshipTarget] = metrics = [];
                var oldValue = metrics.GetValueOrDefault(selected.RelationshipMetric);
                var newValue = oldValue + selected.RelationshipDelta;
                metrics[selected.RelationshipMetric] = newValue;
                var relationship = MakeEvent("AI_NPC_CHANGED_RELATIONSHIP", npc.NpcId, "AI_NPC",
                    relationshipTarget, "NPC", npc.CurrentLocationId);
                relationship.SourceEventId = perception.EventId;
                relationship.RootEventId = perception.RootEventId;
                relationship.Payload["metric"] = selected.RelationshipMetric;
                relationship.Payload["old_value"] = oldValue.ToString(CultureInfo.InvariantCulture);
                relationship.Payload["new_value"] = newValue.ToString(CultureInfo.InvariantCulture);
                relationship.Payload["reason"] = $"transition_rule:{selected.RuleId}";
                generated.Add(relationship);
            }
        }

        var changed = MakeEvent("AI_NPC_CHANGED_STATE", npc.NpcId, "AI_NPC", npc.NpcId, "AI_NPC",
            npc.CurrentLocationId);
        changed.SourceEventId = perception.EventId;
        changed.RootEventId = perception.RootEventId;
        changed.Payload["old_state"] = oldState.ToString(CultureInfo.InvariantCulture);
        changed.Payload["new_state"] = npc.CurrentStateId.ToString(CultureInfo.InvariantCulture);
        changed.Payload["selected_rule"] = selected.RuleId;
        generated.Add(changed);
        if (action is null)
        {
            audit.GeneratedEvents.AddRange(generated.Select(value => value.EventId));
            return generated;
        }

        npc.ActiveAction = action.ActionType;
        var started = MakeEvent("AI_NPC_STARTED_ACTION", npc.NpcId, "AI_NPC", action.TargetId,
            "ACTION_TARGET", npc.CurrentLocationId);
        started.SourceEventId = perception.EventId;
        started.RootEventId = perception.RootEventId;
        started.Payload["action_type"] = action.ActionType;
        generated.Add(started);
        var completed = MakeEvent("AI_NPC_COMPLETED_ACTION", npc.NpcId, "AI_NPC", action.TargetId,
            "ACTION_TARGET", npc.CurrentLocationId);
        completed.SourceEventId = started.EventId;
        completed.RootEventId = perception.RootEventId;
        completed.Payload["action_type"] = action.ActionType;
        generated.Add(completed);
        npc.ActiveAction = "";

        if (action.ActionType == "REPORT" && !string.IsNullOrEmpty(action.TargetId))
        {
            var heard = MakeEvent("AI_NPC_HEARD_EVENT", npc.NpcId, "AI_NPC", action.TargetId, "AI_NPC",
                AiNpc(action.TargetId).CurrentLocationId);
            heard.SourceEventId = completed.EventId;
            heard.RootEventId = perception.RootEventId;
            heard.Credibility = Math.Clamp(perception.Credibility * action.CredibilityFactor, 0.0, 1.0);
            heard.EvidenceLevel = perception.EvidenceLevel;
            heard.Payload["subject_event_id"] = perception.Payload["subject_event_id"];
            heard.Payload["subject_event_type"] = perception.Payload["subject_event_type"];
            heard.Payload["perception"] = "HEARD";
            heard.Payload["reported_by"] = npc.NpcId;
            generated.Add(heard);
        }

        if (action.ActionType is "ARREST" or "WARN" or "PROTECT" or "HELP")
        {
            var intervened = MakeEvent("AI_NPC_INTERVENED", npc.NpcId, "AI_NPC",
                perception.Payload.GetValueOrDefault("subject_event_id", ""), "EVENT", npc.CurrentLocationId);
            intervened.SourceEventId = completed.EventId;
            intervened.RootEventId = perception.RootEventId;
            intervened.Payload["action_type"] = action.ActionType;
            generated.Add(intervened);
            if (action.ActionType == "ARREST") Player.CrimeRecord++;
        }

        foreach (var effect in action.CountryEffects)
        {
            var oldValue = CountryParameter(effect.Parameter);
            SetCountryParameter(effect.Parameter, oldValue + effect.Delta);
            Country.UpdatedAt = CurrentWorldTimeMinutes;
            var countryEvent = MakeEvent("COUNTRY_STATE_CHANGED", npc.NpcId, "AI_NPC", Country.CountryId,
                "COUNTRY", npc.CurrentLocationId);
            countryEvent.SourceEventId = completed.EventId;
            countryEvent.RootEventId = perception.RootEventId;
            countryEvent.Payload["parameter"] = effect.Parameter;
            countryEvent.Payload["old_value"] = oldValue.ToString(CultureInfo.InvariantCulture);
            countryEvent.Payload["new_value"] = (oldValue + effect.Delta).ToString(CultureInfo.InvariantCulture);
            countryEvent.Payload["reason"] = effect.Reason;
            audit.CountryStateChanges.Add($"{effect.Parameter}:{oldValue}->{oldValue + effect.Delta}");
            generated.Add(countryEvent);
        }
        audit.GeneratedEvents.AddRange(generated.Select(value => value.EventId));
        return generated;
    }

    public OfflineResult AdvanceOffline(long elapsedRealSeconds)
    {
        if (elapsedRealSeconds < 0) throw new ArgumentOutOfRangeException(nameof(elapsedRealSeconds));
        var result = new OfflineResult
        {
            RequestedRealSeconds = elapsedRealSeconds,
            AppliedRealSeconds = Math.Min(elapsedRealSeconds, _offlineLimitSeconds)
        };
        result.ElapsedGameMinutes = result.AppliedRealSeconds * 60 / _realSecondsPerGameHour;
        var statesBefore = AiNpcs.ToDictionary(npc => npc.NpcId, npc => npc.CurrentStateId, StringComparer.Ordinal);
        var historyStart = EventHistory.Count;
        var elapsed = MakeEvent("TIME_ELAPSED", "world_001", "WORLD", "", "", Player.CurrentLocationId);
        elapsed.RootEventId = elapsed.EventId;
        elapsed.Payload["game_minutes"] = result.ElapsedGameMinutes.ToString(CultureInfo.InvariantCulture);
        elapsed.Payload["real_seconds"] = result.AppliedRealSeconds.ToString(CultureInfo.InvariantCulture);
        elapsed.Payload["requested_real_seconds"] = elapsedRealSeconds.ToString(CultureInfo.InvariantCulture);
        elapsed.Payload["offline"] = "true";
        LastSimulatedAt += result.AppliedRealSeconds;
        PendingEvents.Enqueue(elapsed);
        ProcessPending();
        foreach (var value in EventHistory.Skip(historyStart))
        {
            result.ImportantEvents.Add($"{value.EventId}:{value.EventType}");
            if (value.EventType == "COUNTRY_STATE_CHANGED") result.CountryChanges.Add(value.EventId);
        }
        foreach (var npc in AiNpcs)
            if (statesBefore[npc.NpcId] != npc.CurrentStateId) result.ChangedAiNpcs.Add(npc.NpcId);
        result.UnresolvedEvents.AddRange(PendingEvents.Select(value => value.EventId));
        return result;
    }

    public void Save(string savePath)
    {
        var document = CreateSaveDocument();
        var temporary = savePath + ".tmp";
        var backup = savePath + ".bak";
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(savePath))!);
        var options = new JsonSerializerOptions(JsonOptions) { WriteIndented = true };
        File.WriteAllText(temporary, JsonSerializer.Serialize(document, options) + Environment.NewLine, Encoding.UTF8);
        if (File.Exists(backup)) File.Delete(backup);
        var hadPrevious = File.Exists(savePath);
        try
        {
            if (hadPrevious) File.Move(savePath, backup);
            File.Move(temporary, savePath);
            if (File.Exists(backup)) File.Delete(backup);
        }
        catch
        {
            if (hadPrevious && File.Exists(backup) && !File.Exists(savePath)) File.Move(backup, savePath);
            if (File.Exists(temporary)) File.Delete(temporary);
            throw;
        }
    }

    public static Simulation LoadSave(string fixturePath, string savePath)
    {
        var simulation = FromFixture(fixturePath);
        var document = JsonSerializer.Deserialize<SaveDocument>(File.ReadAllText(savePath, Encoding.UTF8), JsonOptions)
            ?? throw new InvalidDataException("Save JSON deserialized to null.");
        if (document.FixtureId != simulation._fixtureId || document.SchemaVersion != simulation._schemaVersion ||
            document.SimulationVersion != simulation._simulationVersion)
            throw new InvalidDataException("Save version or fixture mismatch.");
        simulation.WorldSeed = document.WorldSeed;
        simulation.CurrentWorldTimeMinutes = document.CurrentWorldTimeMinutes;
        simulation.LastSimulatedAt = document.LastSimulatedAt;
        simulation.SimulationTick = document.SimulationTick;
        simulation._nextEventSequence = document.NextEventSequence;
        simulation.ChainLimit = document.ChainLimit;
        simulation.Country = document.Country;
        simulation.Player = document.Player;
        foreach (var runtime in document.AiRuntime)
        {
            var npc = simulation.AiNpc(runtime.NpcId);
            npc.CurrentLocationId = runtime.CurrentLocationId;
            npc.CurrentStateId = runtime.CurrentStateId;
            if (simulation.StateDefinition(npc, npc.CurrentStateId).Undefined)
                throw new InvalidDataException("Save attempted to load an UNDEFINED AI state.");
            npc.PlayerEvaluation = runtime.PlayerEvaluation;
            npc.Relationships = runtime.Relationships;
            npc.CurrentGoal = runtime.CurrentGoal;
            npc.KnownEvents = runtime.KnownEvents;
            npc.ActiveAction = runtime.ActiveAction;
            npc.Status = runtime.Status;
            npc.UsedRules = runtime.UsedRules;
            npc.RuleLastUsedTick = runtime.RuleLastUsedTick;
            npc.UsedDialogues = runtime.UsedDialogues;
            npc.DialogueLastUsedTick = runtime.DialogueLastUsedTick;
            npc.UpdatedAt = runtime.UpdatedAt;
        }
        foreach (var runtime in document.NonAiRuntime)
        {
            var npc = simulation.NonAiNpc(runtime.NpcId);
            npc.CurrentLocationId = runtime.CurrentLocationId;
            npc.CurrentActivity = runtime.CurrentActivity;
            npc.TemporaryMemory = runtime.TemporaryMemory;
            npc.SpawnedAt = runtime.SpawnedAt;
            npc.DespawnPolicy = runtime.DespawnPolicy;
        }
        simulation.EventHistory.AddRange(document.EventHistory);
        simulation.AuditLog.AddRange(document.AuditLog);
        foreach (var pending in document.PendingEvents) simulation.PendingEvents.Enqueue(pending);
        foreach (var pair in document.RootProcessedCounts) simulation._rootProcessedCounts[pair.Key] = pair.Value;
        return simulation;
    }

    public List<string> CausalPath(string eventId)
    {
        List<string> path = [];
        HashSet<string> visited = new(StringComparer.Ordinal);
        var current = eventId;
        while (!string.IsNullOrEmpty(current))
        {
            if (!visited.Add(current)) throw new InvalidOperationException($"Causal path cycle at {current}.");
            var value = Event(current);
            path.Add($"{value.EventId}:{value.EventType}");
            current = value.SourceEventId;
        }
        path.Reverse();
        return path;
    }

    public string CanonicalSnapshot() => JsonSerializer.Serialize(CreateSaveDocument() with
    {
        FixtureId = "",
        SchemaVersion = "",
        SimulationVersion = ""
    }, JsonOptions);

    public string AuditLogJsonLines() => string.Join('\n', AuditLog.Select(value => JsonSerializer.Serialize(value, JsonOptions))) +
        (AuditLog.Count > 0 ? "\n" : "");

    public string CausalLogJsonLines() => string.Join('\n', EventHistory.Select(value => JsonSerializer.Serialize(new
    {
        value.EventId,
        value.EventType,
        value.SourceEventId,
        value.RootEventId,
        value.ActorId,
        value.TargetId
    }, JsonOptions))) + (EventHistory.Count > 0 ? "\n" : "");

    public void SetChainLimitForTest(int limit)
    {
        if (limit <= 0) throw new ArgumentOutOfRangeException(nameof(limit));
        ChainLimit = limit;
    }

    private SaveDocument CreateSaveDocument() => new()
    {
        FixtureId = _fixtureId,
        SchemaVersion = _schemaVersion,
        SimulationVersion = _simulationVersion,
        WorldSeed = WorldSeed,
        CurrentWorldTimeMinutes = CurrentWorldTimeMinutes,
        LastSimulatedAt = LastSimulatedAt,
        SimulationTick = SimulationTick,
        NextEventSequence = _nextEventSequence,
        ChainLimit = ChainLimit,
        Country = Country,
        Player = Player,
        AiRuntime = AiNpcs.Select(AiRuntime.FromNpc).ToList(),
        NonAiRuntime = NonAiNpcs.Select(NonAiRuntime.FromNpc).ToList(),
        EventHistory = EventHistory,
        AuditLog = AuditLog,
        PendingEvents = PendingEvents.ToList(),
        RootProcessedCounts = new Dictionary<string, int>(_rootProcessedCounts, StringComparer.Ordinal)
    };

    private void AppendHistory(SimEvent simEvent)
    {
        simEvent.SimulationTick = ++SimulationTick;
        EventHistory.Add(simEvent);
    }

    private void AppendChainLimitEvent(string rootEventId, int remaining, string lastActor)
    {
        var limit = MakeEvent("CHAIN_LIMIT_REACHED", "simulation", "SYSTEM", "", "", Player.CurrentLocationId);
        limit.RootEventId = rootEventId;
        limit.Payload["root_event_id"] = rootEventId;
        limit.Payload["processed_event_count"] = _rootProcessedCounts[rootEventId].ToString(CultureInfo.InvariantCulture);
        limit.Payload["remaining_event_count"] = remaining.ToString(CultureInfo.InvariantCulture);
        limit.Payload["last_processed_actor"] = lastActor;
        AppendHistory(limit);
    }

    private SimEvent MakeEvent(string type, string actorId, string actorType, string targetId,
        string targetType, string locationId)
    {
        var value = new SimEvent
        {
            EventId = $"evt_{_nextEventSequence++:0000000000}",
            EventType = type,
            ActorId = actorId,
            ActorType = actorType,
            TargetId = targetId,
            TargetType = targetType,
            LocationId = locationId,
            CountryId = Country.CountryId,
            OccurredAt = CurrentWorldTimeMinutes,
            Participants = [actorId]
        };
        if (!string.IsNullOrEmpty(targetId)) value.Participants.Add(targetId);
        return value;
    }

    private StateDefinition StateDefinition(AiNpcState npc, int stateId)
    {
        if (stateId is < 1 or > 255) throw new ArgumentOutOfRangeException(nameof(stateId));
        return npc.States[stateId - 1];
    }

    private int CountryParameter(string parameter) => parameter switch
    {
        "stability" => Country.Stability,
        "security" => Country.Security,
        "economy" => Country.Economy,
        "food" => Country.Food,
        "military" => Country.Military,
        "public_support" => Country.PublicSupport,
        "authority" => Country.Authority,
        "crime_level" => Country.CrimeLevel,
        _ => throw new ArgumentException($"Unsupported country parameter: {parameter}")
    };

    private void SetCountryParameter(string parameter, int value)
    {
        switch (parameter)
        {
            case "stability": Country.Stability = value; break;
            case "security": Country.Security = value; break;
            case "economy": Country.Economy = value; break;
            case "food": Country.Food = value; break;
            case "military": Country.Military = value; break;
            case "public_support": Country.PublicSupport = value; break;
            case "authority": Country.Authority = value; break;
            case "crime_level": Country.CrimeLevel = value; break;
            default: throw new ArgumentException($"Unsupported country parameter: {parameter}");
        }
    }

    private static string ActionEventType(string action, bool targetIsAi) => action switch
    {
        "HELP" => targetIsAi ? "PLAYER_HELPED_AI_NPC" : "PLAYER_HELPED_NON_AI_NPC",
        "HARM" => targetIsAi ? "PLAYER_HARMED_AI_NPC" : "PLAYER_HARMED_NON_AI_NPC",
        "TALK" => "PLAYER_TALKED",
        "TRADE" => "PLAYER_TRADED",
        "STEAL" => "PLAYER_STOLE",
        "MOVE" => "PLAYER_LEFT_LOCATION",
        "WAIT" => "TIME_ELAPSED",
        _ => throw new ArgumentException($"Unsupported action: {action}")
    };

    private static double EventSalience(string eventType) => eventType switch
    {
        "PLAYER_HARMED_NON_AI_NPC" or "PLAYER_HARMED_AI_NPC" => 0.95,
        "PLAYER_STOLE" => 0.85,
        "PLAYER_HELPED_NON_AI_NPC" or "PLAYER_HELPED_AI_NPC" => 0.65,
        "PLAYER_TRADED" => 0.50,
        "PLAYER_TALKED" => 0.35,
        _ => 0.40
    };

    private static ulong DeterministicHash(string value)
    {
        const ulong offset = 14695981039346656037UL;
        const ulong prime = 1099511628211UL;
        var hash = offset;
        foreach (var octet in Encoding.UTF8.GetBytes(value))
        {
            hash ^= octet;
            hash *= prime;
        }
        return hash;
    }
}

public sealed record SaveDocument
{
    public string FixtureId { get; init; } = "";
    public string SchemaVersion { get; init; } = "";
    public string SimulationVersion { get; init; } = "";
    public ulong WorldSeed { get; init; }
    public long CurrentWorldTimeMinutes { get; init; }
    public long LastSimulatedAt { get; init; }
    public long SimulationTick { get; init; }
    public ulong NextEventSequence { get; init; }
    public int ChainLimit { get; init; }
    public CountryState Country { get; init; } = new();
    public PlayerState Player { get; init; } = new();
    public List<AiRuntime> AiRuntime { get; init; } = [];
    public List<NonAiRuntime> NonAiRuntime { get; init; } = [];
    public List<SimEvent> EventHistory { get; init; } = [];
    public List<AuditEntry> AuditLog { get; init; } = [];
    public List<SimEvent> PendingEvents { get; init; } = [];
    public Dictionary<string, int> RootProcessedCounts { get; init; } = [];
}

public sealed class AiRuntime
{
    public string NpcId { get; set; } = "";
    public string CurrentLocationId { get; set; } = "";
    public int CurrentStateId { get; set; }
    public int PlayerEvaluation { get; set; }
    public Dictionary<string, Dictionary<string, int>> Relationships { get; set; } = [];
    public string CurrentGoal { get; set; } = "";
    public List<string> KnownEvents { get; set; } = [];
    public string ActiveAction { get; set; } = "";
    public string Status { get; set; } = "";
    public HashSet<string> UsedRules { get; set; } = [];
    public Dictionary<string, long> RuleLastUsedTick { get; set; } = [];
    public HashSet<string> UsedDialogues { get; set; } = [];
    public Dictionary<string, long> DialogueLastUsedTick { get; set; } = [];
    public long UpdatedAt { get; set; }

    public static AiRuntime FromNpc(AiNpcState npc) => new()
    {
        NpcId = npc.NpcId,
        CurrentLocationId = npc.CurrentLocationId,
        CurrentStateId = npc.CurrentStateId,
        PlayerEvaluation = npc.PlayerEvaluation,
        Relationships = npc.Relationships,
        CurrentGoal = npc.CurrentGoal,
        KnownEvents = npc.KnownEvents,
        ActiveAction = npc.ActiveAction,
        Status = npc.Status,
        UsedRules = npc.UsedRules,
        RuleLastUsedTick = npc.RuleLastUsedTick,
        UsedDialogues = npc.UsedDialogues,
        DialogueLastUsedTick = npc.DialogueLastUsedTick,
        UpdatedAt = npc.UpdatedAt
    };
}

public sealed class NonAiRuntime
{
    public string NpcId { get; set; } = "";
    public string CurrentLocationId { get; set; } = "";
    public string CurrentActivity { get; set; } = "";
    public List<string> TemporaryMemory { get; set; } = [];
    public long SpawnedAt { get; set; }
    public string DespawnPolicy { get; set; } = "";

    public static NonAiRuntime FromNpc(NonAiNpcState npc) => new()
    {
        NpcId = npc.NpcId,
        CurrentLocationId = npc.CurrentLocationId,
        CurrentActivity = npc.CurrentActivity,
        TemporaryMemory = npc.TemporaryMemory,
        SpawnedAt = npc.SpawnedAt,
        DespawnPolicy = npc.DespawnPolicy
    };
}
