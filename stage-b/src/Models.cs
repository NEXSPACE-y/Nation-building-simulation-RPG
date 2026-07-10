using System.Text.Json.Serialization;

namespace NationSimulation.StageB;

public sealed class FixtureDocument
{
    public string FixtureId { get; set; } = "";
    public bool FixtureOnly { get; set; }
    public string SchemaVersion { get; set; } = "";
    public string SimulationVersion { get; set; } = "";
    public WorldConfig World { get; set; } = new();
    public SettingsConfig Settings { get; set; } = new();
    public List<string> AllowedPlayerActions { get; set; } = [];
    public List<string> EventTypes { get; set; } = [];
    public List<LocationDefinition> Locations { get; set; } = [];
    public CountryState Country { get; set; } = new();
    public PlayerState Player { get; set; } = new();
    public List<AiNpcState> AiNpcs { get; set; } = [];
    public List<NonAiNpcState> NonAiNpcs { get; set; } = [];
}

public sealed class WorldConfig
{
    public string WorldId { get; set; } = "";
    public ulong WorldSeed { get; set; }
    public long CurrentWorldTimeMinutes { get; set; }
    public long LastSimulatedAt { get; set; }
}

public sealed class SettingsConfig
{
    public int RealSecondsPerGameHour { get; set; }
    public long OfflineLimitSeconds { get; set; }
    public int ChainLimit { get; set; }
    public double WitnessThreshold { get; set; }
}

public sealed class LocationDefinition
{
    public string LocationId { get; set; } = "";
    public double Visibility { get; set; }
    public double Obstruction { get; set; }
}

public sealed class CountryState
{
    public string CountryId { get; set; } = "";
    public string Name { get; set; } = "";
    public int Stability { get; set; }
    public int Security { get; set; }
    public int Economy { get; set; }
    public int Food { get; set; }
    public int Military { get; set; }
    public int PublicSupport { get; set; }
    public int Authority { get; set; }
    public int CrimeLevel { get; set; }
    public List<string> ActiveIssues { get; set; } = [];
    public long UpdatedAt { get; set; }
}

public sealed class PlayerState
{
    public string PlayerId { get; set; } = "";
    public string CountryId { get; set; } = "";
    public string CurrentLocationId { get; set; } = "";
    public string Status { get; set; } = "";
    public List<string> Inventory { get; set; } = [];
    public int Funds { get; set; }
    public int Reputation { get; set; }
    public int CrimeRecord { get; set; }
    public List<string> AchievementTags { get; set; } = [];
    public List<string> ActionHistory { get; set; } = [];
    public long LastLoginAt { get; set; }
    public long CreatedAt { get; set; }
    public long UpdatedAt { get; set; }
}

public sealed class DialogueCandidate
{
    public string DialogueId { get; set; } = "";
    public string Text { get; set; } = "";
    public string Tone { get; set; } = "";
    public int Priority { get; set; }
    public bool OnceOnly { get; set; }
    public long Cooldown { get; set; }
}

public sealed class CountryEffect
{
    public string Parameter { get; set; } = "";
    public int Delta { get; set; }
    public string Reason { get; set; } = "";
}

public sealed class ActionCandidate
{
    [JsonPropertyName("type")]
    public string ActionType { get; set; } = "";
    public string TargetId { get; set; } = "";
    public int Priority { get; set; }
    public double CredibilityFactor { get; set; } = 1.0;
    public List<CountryEffect> CountryEffects { get; set; } = [];
}

public sealed class TransitionRule
{
    public string RuleId { get; set; } = "";
    public string TriggerEventType { get; set; } = "";
    public string SubjectEventType { get; set; } = "";
    public string Perception { get; set; } = "";
    public double MinCredibility { get; set; }
    public double MaxCredibility { get; set; } = 1.0;
    public int TargetStateId { get; set; }
    public int Priority { get; set; }
    public long Cooldown { get; set; }
    public bool OnceOnly { get; set; }
    public int PlayerEvaluationDelta { get; set; }
    public string RelationshipMetric { get; set; } = "";
    public int RelationshipDelta { get; set; }
    public string RelationshipTarget { get; set; } = "";
}

public sealed class StateDefinition
{
    public int StateId { get; set; }
    public string StateName { get; set; } = "";
    public string StateDescription { get; set; } = "";
    public bool Undefined { get; set; }
    public List<DialogueCandidate> DialogueCandidates { get; set; } = [];
    public List<ActionCandidate> ActionCandidates { get; set; } = [];
    public List<TransitionRule> TransitionRules { get; set; } = [];
    public string GoalModifier { get; set; } = "";
    public int PlayerEvaluationModifier { get; set; }
    public Dictionary<string, int> RelationshipModifiers { get; set; } = [];
    public List<string> WorldEffectCandidates { get; set; } = [];
    public List<string> TimeBasedRules { get; set; } = [];
    public int Priority { get; set; }
    public bool IsTerminal { get; set; }
}

public sealed class AiNpcState
{
    public string NpcId { get; set; } = "";
    public string Name { get; set; } = "";
    public string Role { get; set; } = "";
    public string FactionId { get; set; } = "";
    public string CountryId { get; set; } = "";
    public string CurrentLocationId { get; set; } = "";
    [JsonPropertyName("initial_state_id")]
    public int CurrentStateId { get; set; }
    public int StateSlotCount { get; set; }
    public double Attention { get; set; }
    public double HearingTrustThreshold { get; set; }
    public Dictionary<string, int> PersonalityTraits { get; set; } = [];
    public int PlayerEvaluation { get; set; }
    public Dictionary<string, Dictionary<string, int>> Relationships { get; set; } = [];
    public string CurrentGoal { get; set; } = "";
    public string MemorySummary { get; set; } = "";
    public List<string> KnownEvents { get; set; } = [];
    public string ActiveAction { get; set; } = "";
    public string Status { get; set; } = "";
    public List<StateDefinition> States { get; set; } = [];
    public HashSet<string> UsedRules { get; set; } = [];
    public Dictionary<string, long> RuleLastUsedTick { get; set; } = [];
    public HashSet<string> UsedDialogues { get; set; } = [];
    public Dictionary<string, long> DialogueLastUsedTick { get; set; } = [];
    public long CreatedAt { get; set; }
    public long UpdatedAt { get; set; }
}

public sealed class NonAiNpcState
{
    [JsonPropertyName("non_ai_npc_id")]
    public string NpcId { get; set; } = "";
    public ulong Seed { get; set; }
    public string CountryId { get; set; } = "";
    public string CurrentLocationId { get; set; } = "";
    public string Category { get; set; } = "";
    public string Occupation { get; set; } = "";
    public string BasicProfile { get; set; } = "";
    public string CurrentActivity { get; set; } = "";
    public List<string> TemporaryMemory { get; set; } = [];
    public long SpawnedAt { get; set; }
    public string DespawnPolicy { get; set; } = "";
}

public sealed class SimEvent
{
    public string EventId { get; set; } = "";
    public string EventType { get; set; } = "";
    public string ActorId { get; set; } = "";
    public string ActorType { get; set; } = "";
    public string TargetId { get; set; } = "";
    public string TargetType { get; set; } = "";
    public string LocationId { get; set; } = "";
    public string CountryId { get; set; } = "";
    public long OccurredAt { get; set; }
    public long SimulationTick { get; set; }
    public List<string> ObservedBy { get; set; } = [];
    public List<string> HeardBy { get; set; } = [];
    public List<string> Participants { get; set; } = [];
    public double EvidenceLevel { get; set; } = 1.0;
    public double Credibility { get; set; } = 1.0;
    public string SourceEventId { get; set; } = "";
    public string RootEventId { get; set; } = "";
    public Dictionary<string, string> Payload { get; set; } = [];
    public List<string> ProcessedBy { get; set; } = [];
    public List<string> ResultingEvents { get; set; } = [];
}

public sealed class AuditEntry
{
    public long Timestamp { get; set; }
    public long SimulationTick { get; set; }
    public string EventId { get; set; } = "";
    public string RootEventId { get; set; } = "";
    public string EventActorId { get; set; } = "";
    public string DecisionNpcId { get; set; } = "";
    public string TargetId { get; set; } = "";
    public int CurrentState { get; set; }
    public List<string> MatchedRules { get; set; } = [];
    public string SelectedRule { get; set; } = "";
    public int NextState { get; set; }
    public string SelectedDialogue { get; set; } = "";
    public string SelectedAction { get; set; } = "";
    public List<string> GeneratedEvents { get; set; } = [];
    public List<string> CountryStateChanges { get; set; } = [];
    public ulong RandomSeed { get; set; }
}

public sealed class OfflineResult
{
    public long RequestedRealSeconds { get; set; }
    public long AppliedRealSeconds { get; set; }
    public long ElapsedGameMinutes { get; set; }
    public List<string> ImportantEvents { get; set; } = [];
    public List<string> ChangedAiNpcs { get; set; } = [];
    public List<string> CountryChanges { get; set; } = [];
    public List<string> UnresolvedEvents { get; set; } = [];
}
