#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace nation_sim {

struct CountryState {
    std::string country_id;
    std::string name;
    int stability{};
    int security{};
    int economy{};
    int food{};
    int military{};
    int public_support{};
    int authority{};
    int crime_level{};
    std::vector<std::string> active_issues;
    std::int64_t updated_at{};
};

struct PlayerState {
    std::string player_id;
    std::string country_id;
    std::string current_location_id;
    std::string status;
    std::vector<std::string> inventory;
    int funds{};
    int reputation{};
    int crime_record{};
    std::vector<std::string> achievement_tags;
    std::vector<std::string> action_history;
    std::int64_t last_login_at{};
    std::int64_t created_at{};
    std::int64_t updated_at{};
};

struct DialogueCandidate {
    std::string dialogue_id;
    std::string text;
    std::string tone;
    int priority{};
    bool once_only{};
    std::int64_t cooldown{};
};

struct CountryEffect {
    std::string parameter;
    int delta{};
    std::string reason;
};

struct ActionCandidate {
    std::string action_type;
    std::string target_id;
    int priority{};
    double credibility_factor{1.0};
    std::vector<CountryEffect> country_effects;
};

struct TransitionRule {
    std::string rule_id;
    std::string trigger_event_type;
    std::string subject_event_type;
    std::string perception;
    double min_credibility{0.0};
    double max_credibility{1.0};
    double min_evidence{0.0};
    double max_evidence{1.0};
    std::string required_actor_id;
    int target_state_id{};
    int priority{};
    std::int64_t cooldown{};
    bool once_only{};
    int player_evaluation_delta{};
    std::string relationship_metric;
    int relationship_delta{};
    std::string relationship_target;
    int min_relationship{-1000000};
    int max_relationship{1000000};
    std::string country_parameter;
    int min_country{-1000000};
    int max_country{1000000};
    int min_player_evaluation{-1000000};
    int max_player_evaluation{1000000};
    std::int64_t min_state_minutes{};
};

struct TimeBasedRule {
    std::string rule_id;
    std::int64_t after_game_minutes{};
    int target_state_id{};
    int priority{};
    bool once_only{};
    std::int64_t repeat_interval_minutes{};
};

struct StateDefinition {
    int state_id{};
    std::string state_name;
    std::string state_description;
    bool undefined{};
    std::vector<DialogueCandidate> dialogue_candidates;
    std::vector<ActionCandidate> action_candidates;
    std::vector<TransitionRule> transition_rules;
    std::vector<TimeBasedRule> time_based_rules;
    std::string goal_modifier;
    std::string current_goal;
    int player_evaluation_modifier{};
    std::map<std::string, int> relationship_modifiers;
    std::vector<CountryEffect> world_effect_candidates;
    int priority{};
    bool is_terminal{};
};

struct EvidenceEvaluation {
    std::string source_event_id;
    double credibility{};
    double evidence_level{};
    std::string perception{"NONE"};
};

struct RuleEvaluation {
    std::string rule_id;
    bool selected{};
    std::string reason;
};

struct AiNpcState {
    std::string npc_id;
    std::string name;
    std::string role;
    std::string faction_id;
    std::string country_id;
    std::string current_location_id;
    int current_state_id{};
    double attention{};
    double hearing_trust_threshold{};
    std::map<std::string, int> personality_traits;
    int player_evaluation{};
    std::map<std::string, std::map<std::string, int>> relationships;
    std::string current_goal;
    std::string memory_summary;
    std::vector<std::string> known_events;
    std::string active_action;
    std::string status;
    std::vector<StateDefinition> states;
    std::set<std::string> used_rules;
    std::map<std::string, std::int64_t> rule_last_used_tick;
    std::set<std::string> used_dialogues;
    std::map<std::string, std::int64_t> dialogue_last_used_tick;
    std::int64_t state_entered_at{};
    std::optional<std::int64_t> timed_transition_at;
    bool legacy_state_pending_stage_e_entry{};
    EvidenceEvaluation evidence_evaluation;
    std::string last_transition_reason;
    std::vector<RuleEvaluation> last_rule_evaluations;
    std::int64_t created_at{};
    std::int64_t updated_at{};
};

struct NonAiNpcState {
    std::string npc_id;
    std::uint64_t seed{};
    std::string country_id;
    std::string current_location_id;
    std::string category;
    std::string occupation;
    std::string basic_profile;
    std::string current_activity;
    std::vector<std::string> temporary_memory;
    std::int64_t spawned_at{};
    std::string despawn_policy;
};

struct Event {
    std::string event_id;
    std::string event_type;
    std::string actor_id;
    std::string actor_type;
    std::string target_id;
    std::string target_type;
    std::string location_id;
    std::string country_id;
    std::int64_t occurred_at{};
    std::int64_t simulation_tick{};
    std::vector<std::string> observed_by;
    std::vector<std::string> heard_by;
    std::vector<std::string> participants;
    double evidence_level{1.0};
    double credibility{1.0};
    std::string source_event_id;
    std::string root_event_id;
    std::map<std::string, std::string> payload;
    std::vector<std::string> processed_by;
    std::vector<std::string> resulting_events;
};

struct AuditEntry {
    std::int64_t timestamp{};
    std::int64_t simulation_tick{};
    std::string event_id;
    std::string root_event_id;
    std::string event_actor_id;
    std::string decision_npc_id;
    std::string target_id;
    int current_state{};
    std::vector<std::string> matched_rules;
    std::string selected_rule;
    int next_state{};
    std::string selected_dialogue;
    std::string selected_action;
    std::vector<std::string> generated_events;
    std::vector<std::string> country_state_changes;
    std::vector<std::string> rejected_rules;
    std::uint64_t random_seed{};
};

struct OfflineResult {
    std::int64_t requested_real_seconds{};
    std::int64_t applied_real_seconds{};
    std::int64_t elapsed_game_minutes{};
    std::vector<std::string> important_events;
    std::vector<std::string> changed_ai_npcs;
    std::vector<std::string> country_changes;
    std::vector<std::string> unresolved_events;
};

class Simulation {
public:
    static Simulation from_fixture(const std::filesystem::path& fixture_path);
    static Simulation load_save(const std::filesystem::path& fixture_path,
                                const std::filesystem::path& save_path);
    static Simulation from_fixture_with_overlay(const std::filesystem::path& fixture_path,
                                                const std::filesystem::path& overlay_path,
                                                const std::string& definition_sha256 = {});
    static Simulation load_save_with_overlay(const std::filesystem::path& fixture_path,
                                             const std::filesystem::path& overlay_path,
                                             const std::filesystem::path& save_path,
                                             const std::string& definition_sha256 = {});

    std::string enqueue_player_action(const std::string& action,
                                      const std::string& target_id = {},
                                      const std::string& destination_id = {});
    std::string player_action(const std::string& action,
                              const std::string& target_id = {},
                              const std::string& destination_id = {});
    std::string enqueue_rumor(const std::string& non_ai_npc_id,
                              const std::string& target_ai_npc_id,
                              const std::string& original_event_id,
                              double credibility);
    std::string share_rumor(const std::string& non_ai_npc_id,
                            const std::string& target_ai_npc_id,
                            const std::string& original_event_id,
                            double credibility);
    std::size_t process_pending(std::size_t max_events = static_cast<std::size_t>(-1));
    OfflineResult advance_offline(std::int64_t elapsed_real_seconds);

    void save(const std::filesystem::path& save_path) const;
    std::vector<std::string> causal_path(const std::string& event_id) const;
    std::string canonical_snapshot() const;
    std::string audit_log_json_lines() const;
    std::string causal_log_json_lines() const;

    const CountryState& country() const noexcept { return country_; }
    const PlayerState& player() const noexcept { return player_; }
    const std::vector<AiNpcState>& ai_npcs() const noexcept { return ai_npcs_; }
    const std::vector<NonAiNpcState>& non_ai_npcs() const noexcept { return non_ai_npcs_; }
    const std::vector<Event>& event_history() const noexcept { return event_history_; }
    const std::vector<AuditEntry>& audit_log() const noexcept { return audit_log_; }
    const std::deque<Event>& pending_events() const noexcept { return pending_events_; }
    const AiNpcState& ai_npc(const std::string& npc_id) const;
    const Event& event(const std::string& event_id) const;
    std::int64_t current_world_time_minutes() const noexcept { return current_world_time_minutes_; }
    std::int64_t simulation_tick() const noexcept { return simulation_tick_; }
    std::uint64_t world_seed() const noexcept { return world_seed_; }
    int chain_limit() const noexcept { return chain_limit_; }
    bool stage_e_active() const noexcept { return stage_e_active_; }
    const std::string& stage_e_overlay_id() const noexcept { return stage_e_overlay_id_; }
    const std::string& stage_e_definition_sha256() const noexcept { return stage_e_definition_sha256_; }
    void set_relationship_metric(const std::string& npc_id, const std::string& target_id,
                                 const std::string& metric, int value);
    void set_country_parameter(const std::string& parameter, int value);
    void configure_stage_e_migration(std::string source_schema_version,
                                     std::string source_simulation_version,
                                     std::string source_save_sha256,
                                     std::string source_metadata_sha256,
                                     std::string source_backup_path,
                                     std::string metadata_backup_path);
    void set_chain_limit_for_test(int limit);

private:
    std::filesystem::path fixture_path_;
    std::string fixture_id_;
    std::string schema_version_;
    std::string simulation_version_;
    std::string fixture_schema_version_;
    std::string fixture_simulation_version_;
    bool stage_e_active_{};
    std::string stage_e_overlay_id_;
    std::string stage_e_overlay_schema_version_;
    std::string stage_e_definition_sha256_;
    std::string stage_e_source_schema_version_;
    std::string stage_e_source_simulation_version_;
    std::string stage_e_source_save_sha256_;
    std::string stage_e_source_metadata_sha256_;
    std::string stage_e_source_backup_path_;
    std::string stage_e_metadata_backup_path_;
    std::uint64_t world_seed_{};
    std::int64_t current_world_time_minutes_{};
    std::int64_t last_simulated_at_{};
    std::int64_t simulation_tick_{};
    std::uint64_t next_event_sequence_{1};
    int real_seconds_per_game_hour_{60};
    std::int64_t offline_limit_seconds_{604800};
    int chain_limit_{128};
    double witness_threshold_{0.35};
    std::vector<std::string> allowed_actions_;
    std::map<std::string, double> location_visibility_;
    CountryState country_;
    PlayerState player_;
    std::vector<AiNpcState> ai_npcs_;
    std::vector<NonAiNpcState> non_ai_npcs_;
    std::vector<Event> event_history_;
    std::vector<AuditEntry> audit_log_;
    std::deque<Event> pending_events_;
    std::map<std::string, std::size_t> root_processed_counts_;

    std::string next_event_id();
    Event make_event(std::string type, std::string actor_id, std::string actor_type,
                     std::string target_id, std::string target_type,
                     std::string location_id) ;
    std::vector<Event> derive_events(Event& event);
    std::vector<Event> derive_player_event(Event& event);
    std::vector<Event> derive_rumor_event(Event& event);
    std::vector<Event> derive_perception_event(Event& event);
    std::vector<Event> execute_ai_reaction(AiNpcState& npc, const Event& perception,
                                           AuditEntry& audit);
    std::vector<const TransitionRule*> matching_rules(AiNpcState& npc,
                                                      const Event& perception);
    std::string rule_rejection_reason(const AiNpcState& npc, const TransitionRule& rule,
                                      const Event& perception) const;
    const StateDefinition& state_definition(const AiNpcState& npc, int state_id) const;
    AiNpcState& mutable_ai_npc(const std::string& npc_id);
    NonAiNpcState& mutable_non_ai_npc(const std::string& npc_id);
    int* mutable_country_parameter(const std::string& parameter);
    bool is_allowed_action(const std::string& action) const;
    std::uint64_t deterministic_hash(const std::string& value) const;
    void apply_stage_e_overlay(const std::filesystem::path& overlay_path,
                               const std::string& definition_sha256,
                               bool initialize_stage_e_states);
    void ensure_stage_e_defaults(AiNpcState& npc, const std::map<std::string, std::map<std::string, int>>& defaults);
    void append_history(Event event);
    void append_chain_limit_event(const std::string& root_event_id,
                                  std::size_t remaining,
                                  const std::string& last_actor);
};

} // namespace nation_sim
