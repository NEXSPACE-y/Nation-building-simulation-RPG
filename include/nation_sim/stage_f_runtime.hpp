#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nation_sim {

enum class StageFActivityTier {
    active,
    background,
    dormant
};

struct StageFRuntimeCounters {
    std::size_t loaded_country_count{};
    std::size_t ai_npc_total_count{};
    std::size_t active_count{};
    std::size_t background_count{};
    std::size_t dormant_count{};
    std::size_t materialized_non_ai_count{};
    std::size_t promoted_non_ai_count{};
    std::size_t pending_event_count{};
    std::size_t next_due_count{};
    std::uint64_t current_save_generation{};
    std::size_t loaded_state_shard_count{};
    std::size_t state_cache_size{};
    std::int64_t last_offline_duration_seconds{};
    std::int64_t last_save_duration_milliseconds{};
    std::int64_t last_load_duration_milliseconds{};
};

struct StageFMaterializedNonAi {
    std::string npc_id;
    std::string country_id;
    std::string population_class;
    std::uint64_t population_index{};
    std::uint64_t deterministic_seed{};
    std::string occupation_fixture;
    std::string disposition_fixture;
    bool promoted{};
    std::vector<std::string> important_event_ids;
};

struct StageFSaveResult {
    bool success{};
    bool recovered_previous_generation{};
    std::string generation_id;
    std::string parent_generation_id;
    std::string manifest_sha256;
    std::string audit_message;
    std::string error;
};

struct StageFMigrationResult {
    bool success{};
    bool migrated{};
    std::string source_save_sha256;
    std::string source_metadata_sha256;
    std::filesystem::path save_backup_path;
    std::filesystem::path metadata_backup_path;
    std::string error;
};

struct StageFOfflineResult {
    std::int64_t requested_real_seconds{};
    std::int64_t applied_real_seconds{};
    std::int64_t elapsed_game_minutes{};
    std::uint64_t due_actions_processed{};
    std::uint64_t country_intervals_aggregated{};
    std::uint64_t population_intervals_aggregated{};
};

class StageFProductionRuntime final {
public:
    static StageFProductionRuntime load(const std::filesystem::path& scale_data_root,
                                        const std::filesystem::path& log_archive_root = {});
    static StageFProductionRuntime load_generation(const std::filesystem::path& scale_data_root,
                                                   const std::filesystem::path& save_manifest_path,
                                                   std::filesystem::path& out_visible_core_save,
                                                   StageFSaveResult& out_result,
                                                   const std::filesystem::path& log_archive_root = {});
    static StageFMigrationResult migrate_stage_e_save(
        const std::filesystem::path& scale_data_root,
        const std::filesystem::path& save_manifest_path,
        const std::filesystem::path& timestamp_metadata_path,
        std::int64_t saved_at_utc_epoch,
        const std::filesystem::path& log_archive_root = {});

    StageFProductionRuntime(StageFProductionRuntime&&) noexcept;
    StageFProductionRuntime& operator=(StageFProductionRuntime&&) noexcept;
    ~StageFProductionRuntime();
    StageFProductionRuntime(const StageFProductionRuntime&) = delete;
    StageFProductionRuntime& operator=(const StageFProductionRuntime&) = delete;

    const StageFRuntimeCounters& counters() const noexcept;
    const std::string& scale_data_sha256() const noexcept;
    std::uint64_t world_seed() const noexcept;
    std::int64_t current_world_time_minutes() const noexcept;

    std::string state_definition_json(const std::string& npc_id, int state_id);
    std::string enqueue_country_event(const std::string& country_id,
                                      const std::string& actor_id,
                                      const std::string& payload = {});
    std::string enqueue_cross_country_event(const std::string& source_country_id,
                                            const std::string& target_country_id,
                                            const std::string& actor_id,
                                            const std::string& payload = {});
    std::size_t process_pending(std::size_t max_events = static_cast<std::size_t>(-1));
    StageFOfflineResult advance_offline(std::int64_t elapsed_real_seconds);

    StageFMaterializedNonAi materialize_non_ai(const std::string& country_id,
                                               const std::string& population_class,
                                               std::uint64_t population_index);
    void promote_non_ai(const std::string& npc_id, const std::string& important_event_id);
    void dematerialize_non_ai(const std::string& npc_id);
    std::optional<StageFMaterializedNonAi> non_ai(const std::string& npc_id) const;

    StageFSaveResult save_generation(const std::filesystem::path& save_manifest_path,
                                     const std::string& visible_core_save_json,
                                     std::int64_t saved_at_utc_epoch,
                                     const std::string& failure_injection = {});
    std::string canonical_snapshot() const;
    std::string canonical_sha256() const;
    std::vector<std::string> archived_causal_path(const std::string& root_event_id) const;
    void set_all_active_reference_mode(bool enabled);

    static std::string sha256_bytes(const std::string& bytes);
    static std::string sha256_file(const std::filesystem::path& path);

private:
    struct Impl;
    explicit StageFProductionRuntime(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

} // namespace nation_sim
