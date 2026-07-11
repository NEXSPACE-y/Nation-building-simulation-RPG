#include "nation_sim/stage_f_runtime.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <fstream>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace nation_sim {
namespace {
using json = nlohmann::json;

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read file: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

json read_json(const std::filesystem::path& path) {
    return json::parse(read_text(path));
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write file: " + path.string());
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    output.flush();
    if (!output) throw std::runtime_error("cannot flush file: " + path.string());
}

void write_json(const std::filesystem::path& path, const json& value) {
    write_text(path, value.dump(2) + '\n');
}

std::uint32_t rotate_right(std::uint32_t value, std::uint32_t count) {
    return (value >> count) | (value << (32u - count));
}

class Sha256Accumulator {
public:
    void update(const void* bytes, std::size_t size) {
        const auto* input=static_cast<const std::uint8_t*>(bytes);
        total_bytes_+=size;
        while (size>0) {
            const std::size_t copied=std::min(size,buffer_.size()-buffer_size_);
            std::copy_n(input,copied,buffer_.begin()+static_cast<std::ptrdiff_t>(buffer_size_));
            buffer_size_+=copied;
            input+=copied;
            size-=copied;
            if (buffer_size_==buffer_.size()) {
                transform(buffer_.data());
                buffer_size_=0;
            }
        }
    }

    std::string finish() {
        const std::uint64_t bit_length=total_bytes_*8u;
        buffer_[buffer_size_++]=0x80u;
        if (buffer_size_>56u) {
            std::fill(buffer_.begin()+static_cast<std::ptrdiff_t>(buffer_size_),buffer_.end(),std::uint8_t{0});
            transform(buffer_.data());
            buffer_size_=0;
        }
        std::fill(buffer_.begin()+static_cast<std::ptrdiff_t>(buffer_size_),buffer_.begin()+56,std::uint8_t{0});
        for (int index=0;index<8;++index)
            buffer_[56+static_cast<std::size_t>(index)]=static_cast<std::uint8_t>(bit_length>>(56-index*8));
        transform(buffer_.data());
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (const auto value : hash_) output << std::setw(8) << value;
        return output.str();
    }

private:
    void transform(const std::uint8_t* block) {
        static constexpr std::array<std::uint32_t, 64> k{
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
        std::array<std::uint32_t, 64> w{};
        for (int index = 0; index < 16; ++index) {
            const std::size_t base=static_cast<std::size_t>(index)*4u;
            w[index] = (static_cast<std::uint32_t>(block[base]) << 24) |
                       (static_cast<std::uint32_t>(block[base + 1]) << 16) |
                       (static_cast<std::uint32_t>(block[base + 2]) << 8) |
                       static_cast<std::uint32_t>(block[base + 3]);
        }
        for (int index = 16; index < 64; ++index) {
            const auto s0 = rotate_right(w[index - 15], 7) ^ rotate_right(w[index - 15], 18) ^ (w[index - 15] >> 3);
            const auto s1 = rotate_right(w[index - 2], 17) ^ rotate_right(w[index - 2], 19) ^ (w[index - 2] >> 10);
            w[index] = w[index - 16] + s0 + w[index - 7] + s1;
        }
        auto a=hash_[0], b=hash_[1], c=hash_[2], d=hash_[3], e=hash_[4], f=hash_[5], g=hash_[6], h=hash_[7];
        for (int index = 0; index < 64; ++index) {
            const auto s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
            const auto choice = (e & f) ^ ((~e) & g);
            const auto t1 = h + s1 + choice + k[index] + w[index];
            const auto s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + majority;
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        hash_[0]+=a; hash_[1]+=b; hash_[2]+=c; hash_[3]+=d;
        hash_[4]+=e; hash_[5]+=f; hash_[6]+=g; hash_[7]+=h;
    }

    std::array<std::uint32_t,8> hash_{0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
                                      0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::array<std::uint8_t,64> buffer_{};
    std::size_t buffer_size_{};
    std::uint64_t total_bytes_{};
};

std::string sha256_impl(const std::string& input) {
    Sha256Accumulator hash;
    hash.update(input.data(),input.size());
    return hash.finish();
}

std::string sha256_file_impl(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if (!input) throw std::runtime_error("cannot read file for SHA-256: "+path.string());
    Sha256Accumulator hash;
    std::array<char,65536> buffer{};
    while (input) {
        input.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));
        const auto count=input.gcount();
        if (count>0) hash.update(buffer.data(),static_cast<std::size_t>(count));
    }
    if (!input.eof()) throw std::runtime_error("cannot hash file: "+path.string());
    return hash.finish();
}

std::string tier_name(StageFActivityTier tier) {
    switch (tier) {
    case StageFActivityTier::active: return "ACTIVE";
    case StageFActivityTier::background: return "BACKGROUND";
    case StageFActivityTier::dormant: return "DORMANT";
    }
    throw std::runtime_error("invalid activity tier");
}

StageFActivityTier parse_tier(const std::string& value) {
    if (value == "ACTIVE") return StageFActivityTier::active;
    if (value == "BACKGROUND") return StageFActivityTier::background;
    if (value == "DORMANT") return StageFActivityTier::dormant;
    throw std::runtime_error("unknown activity tier: " + value);
}

std::string padded(std::uint64_t value, int width) {
    std::ostringstream output;
    output << std::setw(width) << std::setfill('0') << value;
    return output.str();
}

bool immutable_copy(const std::filesystem::path& source, const std::filesystem::path& target,
                    const std::string& expected_hash, std::string& error) {
    try {
        if (std::filesystem::exists(target)) {
            if (sha256_file_impl(target) != expected_hash) {
                error = "immutable backup has different content: " + target.string();
                return false;
            }
            return true;
        }
        std::filesystem::create_directories(target.parent_path());
        std::filesystem::copy_file(source, target, std::filesystem::copy_options::none);
        if (sha256_file_impl(target) != expected_hash) {
            error = "immutable backup verification failed: " + target.string();
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}
}

struct StageFProductionRuntime::Impl {
    struct AiRuntime {
        std::string npc_id;
        std::string country_id;
        std::string current_location_id;
        std::string role;
        std::string faction_id;
        int current_state_id{1};
        int player_evaluation{};
        std::map<std::string, std::map<std::string, int>> relationships_summary;
        std::string current_goal;
        std::vector<std::string> known_event_summary;
        std::string active_action;
        StageFActivityTier activity_tier{StageFActivityTier::dormant};
        std::optional<std::int64_t> next_due_time;
        std::string status{"ACTIVE"};
        std::uint64_t version{1};
        std::uint64_t due_action_count{};
    };

    struct CountryRuntime {
        std::string country_id;
        std::uint64_t local_events{};
        std::uint64_t cross_country_events_received{};
        std::uint64_t interval_count{};
        std::string last_root_event_id;
    };

    struct Event {
        std::string event_id;
        std::string root_event_id;
        std::string source_country_id;
        std::string target_country_id;
        std::string actor_id;
        std::string event_type;
        std::string payload;
        std::int64_t scheduled_at{};
        int priority{100};
    };

    struct EventLess {
        bool operator()(const Event& left, const Event& right) const {
            return std::tie(left.scheduled_at, left.target_country_id, left.priority, left.event_id, left.actor_id) <
                   std::tie(right.scheduled_at, right.target_country_id, right.priority, right.event_id, right.actor_id);
        }
    };

    struct Due {
        std::int64_t time{};
        std::string country_id;
        std::string npc_id;
        bool operator<(const Due& other) const {
            return std::tie(time, country_id, npc_id) < std::tie(other.time, other.country_id, other.npc_id);
        }
    };

    struct StateIndexEntry {
        std::filesystem::path relative_path;
        std::string sha256;
        int slot_count{};
    };

    struct Segment {
        std::string segment_id;
        std::string country_id;
        std::string first_event_id;
        std::string last_event_id;
        std::uint64_t first_tick{};
        std::uint64_t last_tick{};
        std::size_t entry_count{};
        std::string sha256;
        std::filesystem::path path;
        std::string in_memory;
    };

    std::filesystem::path scale_root;
    std::filesystem::path log_root;
    std::string scale_hash;
    std::uint64_t world_seed{};
    std::int64_t world_time{};
    std::int64_t offline_limit{604800};
    std::int64_t background_interval{360};
    std::size_t cache_capacity{32};
    std::size_t segment_limit{1000};
    std::uint64_t next_event_sequence{1};
    std::uint64_t simulation_tick{};
    std::uint64_t save_generation{};
    bool all_active_reference_mode{};
    std::uint64_t population_interval_count{};
    std::size_t active_count{};
    std::size_t background_count{};
    std::size_t dormant_count{};
    std::map<std::string, CountryRuntime> countries;
    std::map<std::string, AiRuntime> ai;
    std::map<std::string, std::set<std::string>> location_partition;
    std::set<std::string> active_set;
    std::set<Due> due_queue;
    std::multiset<Event, EventLess> pending;
    std::unordered_map<std::string, StateIndexEntry> state_index;
    std::unordered_map<std::string, json> state_cache;
    std::list<std::string> cache_lru;
    std::size_t loaded_shards{};
    std::map<std::string, StageFMaterializedNonAi> materialized;
    std::map<std::string, StageFMaterializedNonAi> promoted;
    std::vector<json> hot_log;
    std::vector<Segment> segments;
    json migration = json::object();
    StageFRuntimeCounters counters;

    void refresh_counters() {
        counters.loaded_country_count = countries.size();
        counters.ai_npc_total_count = ai.size();
        counters.active_count = active_count;
        counters.background_count = background_count;
        counters.dormant_count = dormant_count;
        counters.materialized_non_ai_count = materialized.size();
        counters.promoted_non_ai_count = promoted.size();
        counters.pending_event_count = pending.size();
        counters.next_due_count = due_queue.size();
        counters.current_save_generation = save_generation;
        counters.loaded_state_shard_count = loaded_shards;
        counters.state_cache_size = state_cache.size();
    }

    std::string next_event_id() {
        return "stage_f_evt_" + padded(next_event_sequence++, 12);
    }

    void touch_cache(const std::string& npc_id) {
        cache_lru.remove(npc_id);
        cache_lru.push_front(npc_id);
        while (state_cache.size() > cache_capacity) {
            const std::string victim = cache_lru.back();
            cache_lru.pop_back();
            state_cache.erase(victim);
        }
    }

    void flush_log_segment() {
        if (hot_log.empty()) return;
        const std::string segment_id = "stage_f_segment_" + padded(segments.size() + 1u, 8);
        std::ostringstream rows;
        for (const auto& row : hot_log) rows << row.dump() << '\n';
        Segment segment;
        segment.segment_id = segment_id;
        segment.country_id = hot_log.front().value("country_id", "MULTI");
        for (const auto& row:hot_log)
            if (row.value("country_id",segment.country_id)!=segment.country_id) {
                segment.country_id="MULTI";
                break;
            }
        segment.first_event_id = hot_log.front().value("event_id", "");
        segment.last_event_id = hot_log.back().value("event_id", "");
        segment.first_tick = hot_log.front().value("simulation_tick", std::uint64_t{});
        segment.last_tick = hot_log.back().value("simulation_tick", std::uint64_t{});
        segment.entry_count = hot_log.size();
        segment.in_memory = rows.str();
        segment.sha256 = sha256_impl(segment.in_memory);
        if (!log_root.empty()) {
            segment.path = log_root / (segment_id + ".jsonl");
            write_text(segment.path, segment.in_memory);
            segment.in_memory.clear();
            segment.in_memory.shrink_to_fit();
        }
        segments.push_back(std::move(segment));
        hot_log.clear();
    }

    json ai_to_json(const AiRuntime& value) const {
        return {{"npc_id",value.npc_id},{"country_id",value.country_id},
                {"current_location_id",value.current_location_id},{"role",value.role},
                {"faction_id",value.faction_id},{"current_state_id",value.current_state_id},
                {"player_evaluation",value.player_evaluation},{"relationships_summary",value.relationships_summary},
                {"current_goal",value.current_goal},{"known_event_summary",value.known_event_summary},
                {"active_action",value.active_action},{"activity_tier",tier_name(value.activity_tier)},
                {"next_due_time",value.next_due_time ? json(*value.next_due_time) : json(nullptr)},
                {"status",value.status},{"version",value.version},{"due_action_count",value.due_action_count}};
    }

    AiRuntime ai_from_json(const json& value) const {
        AiRuntime result;
        result.npc_id=value.at("npc_id"); result.country_id=value.at("country_id");
        result.current_location_id=value.at("current_location_id"); result.role=value.at("role");
        result.faction_id=value.at("faction_id"); result.current_state_id=value.at("current_state_id");
        result.player_evaluation=value.value("player_evaluation",0);
        result.relationships_summary=value.value("relationships_summary",decltype(result.relationships_summary){});
        result.current_goal=value.value("current_goal","");
        result.known_event_summary=value.value("known_event_summary",std::vector<std::string>{});
        result.active_action=value.value("active_action",""); result.activity_tier=parse_tier(value.at("activity_tier"));
        if (value.contains("next_due_time") && !value.at("next_due_time").is_null()) result.next_due_time=value.at("next_due_time").get<std::int64_t>();
        result.status=value.value("status","ACTIVE"); result.version=value.value("version",std::uint64_t{1});
        result.due_action_count=value.value("due_action_count",std::uint64_t{});
        return result;
    }

    json event_to_json(const Event& value) const {
        return {{"event_id",value.event_id},{"root_event_id",value.root_event_id},
                {"source_country_id",value.source_country_id},{"target_country_id",value.target_country_id},
                {"actor_id",value.actor_id},{"event_type",value.event_type},{"payload",value.payload},
                {"scheduled_at",value.scheduled_at},{"priority",value.priority}};
    }

    Event event_from_json(const json& value) const {
        return {value.at("event_id"),value.at("root_event_id"),value.at("source_country_id"),
                value.at("target_country_id"),value.at("actor_id"),value.at("event_type"),
                value.value("payload",""),value.at("scheduled_at"),value.value("priority",100)};
    }
};

StageFProductionRuntime::StageFProductionRuntime(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
StageFProductionRuntime::StageFProductionRuntime(StageFProductionRuntime&&) noexcept = default;
StageFProductionRuntime& StageFProductionRuntime::operator=(StageFProductionRuntime&&) noexcept = default;
StageFProductionRuntime::~StageFProductionRuntime() = default;

StageFProductionRuntime StageFProductionRuntime::load(const std::filesystem::path& scale_data_root,
                                                      const std::filesystem::path& log_archive_root) {
    const auto started = std::chrono::steady_clock::now();
    auto impl = std::make_unique<Impl>();
    impl->scale_root = scale_data_root;
    impl->log_root = log_archive_root;
    if (!log_archive_root.empty()) std::filesystem::create_directories(log_archive_root);
    const auto world_path = scale_data_root / "world_manifest.json";
    const json world = read_json(world_path);
    if (!world.value("fixture_only", false) || world.value("production_content", true) ||
        world.value("schema_version", "") != "stage_f_world_manifest_v1" ||
        world.value("pack_version", "") != "stage-f-scale-0.1.0")
        throw std::runtime_error("Stage F world manifest schema/boundary mismatch");
    const auto counts = world.at("counts");
    if (counts.at("countries") != 5 || counts.at("ai_npcs") != 2500 || counts.at("state_slots") != 637500 ||
        counts.at("resident_population_slots") != 250000000 || counts.at("mobile_population_slots") != 100000000)
        throw std::runtime_error("Stage F world manifest count mismatch");
    impl->world_seed = world.at("world_seed");
    impl->offline_limit = world.value("offline_limit_real_seconds", 604800);
    impl->background_interval = world.value("background_interval_game_minutes", 360);
    impl->cache_capacity = world.value("state_cache_capacity", 32);
    impl->segment_limit = world.value("log_segment_entry_limit", 1000);
    impl->scale_hash = sha256_file_impl(world_path);

    const auto state_index_path=scale_data_root/world.at("state_index").get<std::string>();
    if (sha256_file_impl(state_index_path)!=world.at("state_index_sha256").get<std::string>())
        throw std::runtime_error("Stage F state index hash mismatch");
    const auto definition_hash_path=scale_data_root/world.at("definition_hash_manifest").get<std::string>();
    if (sha256_file_impl(definition_hash_path)!=world.at("definition_hash_manifest_sha256").get<std::string>())
        throw std::runtime_error("Stage F definition hash manifest mismatch");
    const auto population_path=scale_data_root/world.at("population_spaces").get<std::string>();
    if (sha256_file_impl(population_path)!=world.at("population_spaces_sha256").get<std::string>())
        throw std::runtime_error("Stage F population-space hash mismatch");
    const json population_spaces=read_json(population_path);
    if (population_spaces.value("schema_version","")!="stage_f_population_spaces_v1" ||
        population_spaces.at("resident_population_spaces").size()!=5 ||
        population_spaces.at("mobile_population_space").value("slot_count",0ull)!=100000000ull)
        throw std::runtime_error("Stage F population-space contract mismatch");
    for (const auto& resident:population_spaces.at("resident_population_spaces"))
        if (resident.value("slot_count",0ull)!=50000000ull)
            throw std::runtime_error("Stage F resident population-space count mismatch");
    const json state_index = read_json(state_index_path);
    for (const auto& entry : state_index.at("entries")) {
        Impl::StateIndexEntry index;
        index.relative_path=entry.at("state_shard").get<std::string>();
        index.sha256=entry.at("sha256").get<std::string>();
        index.slot_count=entry.at("state_slot_count").get<int>();
        impl->state_index.emplace(entry.at("npc_id").get<std::string>(), std::move(index));
    }
    if (impl->state_index.size() != 2500) throw std::runtime_error("Stage F state index count mismatch");

    for (const auto& country_ref : world.at("country_manifests")) {
        const json country_manifest = read_json(scale_data_root / country_ref.at("path").get<std::string>());
        if (sha256_file_impl(scale_data_root / country_ref.at("path").get<std::string>()) !=
            country_ref.at("sha256").get<std::string>())
            throw std::runtime_error("Stage F country manifest hash mismatch");
        Impl::CountryRuntime country;
        country.country_id = country_manifest.at("country_id");
        if (country_manifest.value("ai_npc_count",0)!=500 || country_manifest.at("ai_manifests").size()!=500)
            throw std::runtime_error("Stage F country AI distribution mismatch");
        impl->countries.emplace(country.country_id, country);
        for (const auto& ai_ref : country_manifest.at("ai_manifests")) {
            const auto ai_path = scale_data_root / ai_ref.at("path").get<std::string>();
            if (sha256_file_impl(ai_path) != ai_ref.at("sha256").get<std::string>())
                throw std::runtime_error("Stage F AI manifest hash mismatch");
            const json value = read_json(ai_path);
            auto runtime = impl->ai_from_json(value.at("runtime"));
            if (runtime.country_id != country.country_id) throw std::runtime_error("Stage F AI country partition mismatch");
            const std::string id = runtime.npc_id;
            const auto state_entry=impl->state_index.find(id);
            if (state_entry==impl->state_index.end() || value.value("state_slot_count",0)!=255 ||
                value.value("state_shard","")!=state_entry->second.relative_path.generic_string() ||
                value.value("state_shard_sha256","")!=state_entry->second.sha256)
                throw std::runtime_error("Stage F AI state index contract mismatch: "+id);
            impl->location_partition[runtime.current_location_id].insert(id);
            if (runtime.activity_tier == StageFActivityTier::active) impl->active_set.insert(id);
            if (runtime.activity_tier == StageFActivityTier::background && runtime.next_due_time)
                impl->due_queue.insert({*runtime.next_due_time, runtime.country_id, id});
            switch (runtime.activity_tier) {
            case StageFActivityTier::active: ++impl->active_count; break;
            case StageFActivityTier::background: ++impl->background_count; break;
            case StageFActivityTier::dormant: ++impl->dormant_count; break;
            }
            if (!impl->ai.emplace(id, std::move(runtime)).second) throw std::runtime_error("duplicate Stage F AI id");
        }
    }
    if (impl->countries.size()!=5 || impl->ai.size()!=2500) throw std::runtime_error("Stage F runtime load count mismatch");
    impl->refresh_counters();
    impl->counters.last_load_duration_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count();
    return StageFProductionRuntime(std::move(impl));
}

const StageFRuntimeCounters& StageFProductionRuntime::counters() const noexcept { return impl_->counters; }
const std::string& StageFProductionRuntime::scale_data_sha256() const noexcept { return impl_->scale_hash; }
std::uint64_t StageFProductionRuntime::world_seed() const noexcept { return impl_->world_seed; }
std::int64_t StageFProductionRuntime::current_world_time_minutes() const noexcept { return impl_->world_time; }

std::string StageFProductionRuntime::state_definition_json(const std::string& npc_id, int state_id) {
    if (state_id < 1 || state_id > 255) throw std::runtime_error("Stage F state id outside 1..255");
    const auto index = impl_->state_index.find(npc_id);
    if (index == impl_->state_index.end()) throw std::runtime_error("Stage F state index NPC missing: " + npc_id);
    auto cached = impl_->state_cache.find(npc_id);
    if (cached == impl_->state_cache.end()) {
        const auto path = impl_->scale_root / index->second.relative_path;
        const std::string bytes = read_text(path);
        if (sha256_impl(bytes) != index->second.sha256) throw std::runtime_error("Stage F state shard hash mismatch: " + npc_id);
        json shard = json::parse(bytes);
        if (shard.value("npc_id", "") != npc_id || shard.at("states").size() != 255)
            throw std::runtime_error("Stage F state shard identity/count mismatch: " + npc_id);
        cached = impl_->state_cache.emplace(npc_id, std::move(shard)).first;
        ++impl_->loaded_shards;
    }
    impl_->touch_cache(npc_id);
    impl_->refresh_counters();
    return cached->second.at("states").at(static_cast<std::size_t>(state_id - 1)).dump();
}

std::string StageFProductionRuntime::enqueue_country_event(const std::string& country_id,
                                                           const std::string& actor_id,
                                                           const std::string& payload) {
    if (!impl_->countries.contains(country_id)) throw std::runtime_error("unknown Stage F country: " + country_id);
    if (actor_id.starts_with("ai_npc_")) {
        const auto actor=impl_->ai.find(actor_id);
        if (actor==impl_->ai.end() || actor->second.country_id!=country_id)
            throw std::runtime_error("Stage F local event actor partition mismatch");
    }
    const std::string id=impl_->next_event_id();
    impl_->pending.insert({id,id,country_id,country_id,actor_id,"STAGE_F_FIXTURE_SIGNAL",payload,impl_->world_time,100});
    impl_->refresh_counters();
    return id;
}

std::string StageFProductionRuntime::enqueue_cross_country_event(const std::string& source_country_id,
                                                                 const std::string& target_country_id,
                                                                 const std::string& actor_id,
                                                                 const std::string& payload) {
    if (!impl_->countries.contains(source_country_id) || !impl_->countries.contains(target_country_id))
        throw std::runtime_error("unknown Stage F cross-country partition");
    if (actor_id.starts_with("ai_npc_")) {
        const auto actor=impl_->ai.find(actor_id);
        if (actor==impl_->ai.end() || actor->second.country_id!=source_country_id)
            throw std::runtime_error("Stage F cross-country event actor partition mismatch");
    }
    const std::string id=impl_->next_event_id();
    impl_->pending.insert({id,id,source_country_id,target_country_id,actor_id,
                           "STAGE_F_FIXTURE_CROSS_COUNTRY",payload,impl_->world_time,50});
    impl_->refresh_counters();
    return id;
}

std::size_t StageFProductionRuntime::process_pending(std::size_t max_events) {
    std::size_t processed=0;
    while (!impl_->pending.empty() && processed<max_events) {
        const auto it=impl_->pending.begin();
        const Impl::Event event=*it;
        impl_->pending.erase(it);
        auto& target=impl_->countries.at(event.target_country_id);
        if (event.event_type=="STAGE_F_FIXTURE_CROSS_COUNTRY") ++target.cross_country_events_received;
        else {
            if (event.source_country_id!=event.target_country_id) throw std::runtime_error("implicit cross-country event rejected");
            ++target.local_events;
        }
        target.last_root_event_id=event.root_event_id;
        ++impl_->simulation_tick;
        impl_->hot_log.push_back({{"event_id",event.event_id},{"root_event_id",event.root_event_id},
                                  {"country_id",event.target_country_id},{"source_country_id",event.source_country_id},
                                  {"event_type",event.event_type},{"actor_id",event.actor_id},
                                  {"simulation_tick",impl_->simulation_tick},{"payload",event.payload}});
        if (impl_->hot_log.size()>=impl_->segment_limit) impl_->flush_log_segment();
        ++processed;
    }
    impl_->refresh_counters();
    return processed;
}

StageFOfflineResult StageFProductionRuntime::advance_offline(std::int64_t elapsed_real_seconds) {
    if (elapsed_real_seconds<0) throw std::runtime_error("negative Stage F offline duration");
    StageFOfflineResult result;
    result.requested_real_seconds=elapsed_real_seconds;
    result.applied_real_seconds=std::min(elapsed_real_seconds,impl_->offline_limit);
    result.elapsed_game_minutes=result.applied_real_seconds;
    process_pending();
    const auto target=impl_->world_time+result.elapsed_game_minutes;
    if (impl_->all_active_reference_mode) {
        for (auto& [npc_id,npc] : impl_->ai) {
            (void)npc_id;
            if (npc.activity_tier != StageFActivityTier::background || !npc.next_due_time ||
                *npc.next_due_time > target) continue;
            const auto occurrences=static_cast<std::uint64_t>(
                (target-*npc.next_due_time)/impl_->background_interval+1);
            npc.due_action_count+=occurrences;
            result.due_actions_processed+=occurrences;
            npc.next_due_time=*npc.next_due_time+
                static_cast<std::int64_t>(occurrences)*impl_->background_interval;
        }
        impl_->due_queue.clear();
        for (const auto& [npc_id,npc] : impl_->ai)
            if (npc.activity_tier==StageFActivityTier::background && npc.next_due_time)
                impl_->due_queue.insert({*npc.next_due_time,npc.country_id,npc_id});
    } else {
        while (!impl_->due_queue.empty() && impl_->due_queue.begin()->time<=target) {
            const Impl::Due due=*impl_->due_queue.begin();
            impl_->due_queue.erase(impl_->due_queue.begin());
            auto& npc=impl_->ai.at(due.npc_id);
            const auto occurrences=static_cast<std::uint64_t>((target-due.time)/impl_->background_interval+1);
            npc.due_action_count+=occurrences;
            result.due_actions_processed+=occurrences;
            npc.next_due_time=due.time+static_cast<std::int64_t>(occurrences)*impl_->background_interval;
            impl_->due_queue.insert({*npc.next_due_time,npc.country_id,npc.npc_id});
        }
    }
    const std::uint64_t intervals=static_cast<std::uint64_t>(result.elapsed_game_minutes/60);
    for (auto& [id,country] : impl_->countries) { (void)id; country.interval_count+=intervals; }
    impl_->population_interval_count+=intervals;
    result.country_intervals_aggregated=intervals*impl_->countries.size();
    result.population_intervals_aggregated=intervals;
    impl_->world_time=target;
    impl_->counters.last_offline_duration_seconds=result.applied_real_seconds;
    impl_->refresh_counters();
    return result;
}

StageFMaterializedNonAi StageFProductionRuntime::materialize_non_ai(const std::string& country_id,
                                                                    const std::string& population_class,
                                                                    std::uint64_t population_index) {
    if (!impl_->countries.contains(country_id)) throw std::runtime_error("unknown NON AI population country");
    if (population_class!="RESIDENT" && population_class!="MOBILE")
        throw std::runtime_error("unknown NON AI population class");
    const std::uint64_t limit=population_class=="MOBILE" ? 100000000ull : 50000000ull;
    if (population_index>=limit) throw std::runtime_error("NON AI population index outside space");
    const std::string basis=std::to_string(impl_->world_seed)+"|"+country_id+"|"+population_class+"|"+std::to_string(population_index);
    const std::string digest=sha256_impl(basis);
    StageFMaterializedNonAi value;
    value.country_id=country_id; value.population_class=population_class; value.population_index=population_index;
    value.deterministic_seed=std::stoull(digest.substr(0,16),nullptr,16);
    value.npc_id="non_ai_f_"+country_id+"_"+population_class+"_"+padded(population_index,9)+"_"+digest.substr(0,12);
    value.occupation_fixture="fixture_occupation_"+std::to_string(value.deterministic_seed%32);
    value.disposition_fixture="fixture_disposition_"+std::to_string((value.deterministic_seed/32)%16);
    if (const auto promoted=impl_->promoted.find(value.npc_id); promoted!=impl_->promoted.end()) return promoted->second;
    if (impl_->materialized.size()>=512) impl_->materialized.erase(impl_->materialized.begin());
    impl_->materialized[value.npc_id]=value;
    impl_->refresh_counters();
    return value;
}

void StageFProductionRuntime::promote_non_ai(const std::string& npc_id, const std::string& important_event_id) {
    auto found=impl_->materialized.find(npc_id);
    if (found==impl_->materialized.end()) throw std::runtime_error("cannot promote non-materialized NON AI NPC");
    auto value=found->second;
    value.promoted=true;
    if (std::find(value.important_event_ids.begin(),value.important_event_ids.end(),important_event_id)==value.important_event_ids.end())
        value.important_event_ids.push_back(important_event_id);
    impl_->promoted[npc_id]=value;
    impl_->materialized.erase(found);
    impl_->refresh_counters();
}

void StageFProductionRuntime::dematerialize_non_ai(const std::string& npc_id) {
    impl_->materialized.erase(npc_id);
    impl_->refresh_counters();
}

std::optional<StageFMaterializedNonAi> StageFProductionRuntime::non_ai(const std::string& npc_id) const {
    if (const auto value=impl_->promoted.find(npc_id); value!=impl_->promoted.end()) return value->second;
    if (const auto value=impl_->materialized.find(npc_id); value!=impl_->materialized.end()) return value->second;
    return std::nullopt;
}

StageFSaveResult StageFProductionRuntime::save_generation(const std::filesystem::path& save_manifest_path,
                                                          const std::string& visible_core_save_json,
                                                          std::int64_t saved_at_utc_epoch,
                                                          const std::string& failure_injection) {
    const auto started=std::chrono::steady_clock::now();
    StageFSaveResult result;
    try {
        const auto parsed_visible_core = json::parse(visible_core_save_json);
        (void)parsed_visible_core;
        const auto generation_root=save_manifest_path.parent_path()/"stage_f_generations";
        std::filesystem::create_directories(generation_root);
        std::uint64_t next=impl_->save_generation+1;
        while (std::filesystem::exists(generation_root/("gen_"+padded(next,12))) ||
               std::filesystem::exists(generation_root/("gen_"+padded(next,12)+".tmp"))) ++next;
        result.generation_id="gen_"+padded(next,12);
        result.parent_generation_id=impl_->save_generation?"gen_"+padded(impl_->save_generation,12):"";
        const auto temporary=generation_root/(result.generation_id+".tmp");
        const auto final=generation_root/result.generation_id;
        std::filesystem::create_directories(temporary);
        write_text(temporary/"incomplete_generation.marker","stage_f_incomplete\n");
        if (failure_injection=="BEFORE_SHARD_WRITE") throw std::runtime_error("injected BEFORE_SHARD_WRITE");

        json world{{"world_seed",impl_->world_seed},{"world_time",impl_->world_time},
                   {"next_event_sequence",impl_->next_event_sequence},{"simulation_tick",impl_->simulation_tick},
                   {"population_interval_count",impl_->population_interval_count},{"scale_data_sha256",impl_->scale_hash},
                   {"migration",impl_->migration}};
        write_json(temporary/"world.json",world);
        if (failure_injection=="DURING_SHARD_WRITE") throw std::runtime_error("injected DURING_SHARD_WRITE");

        std::vector<std::filesystem::path> shard_paths{temporary/"world.json"};
        for (const auto& [country_id,country] : impl_->countries) {
            const auto path=temporary/"countries"/(country_id+".json");
            write_json(path,{{"country_id",country.country_id},{"local_events",country.local_events},
                             {"cross_country_events_received",country.cross_country_events_received},
                             {"interval_count",country.interval_count},{"last_root_event_id",country.last_root_event_id}});
            shard_paths.push_back(path);
            json ai_values=json::array();
            for (const auto& [npc_id,npc] : impl_->ai) if (npc.country_id==country_id) ai_values.push_back(impl_->ai_to_json(npc));
            const auto ai_path=temporary/"ai"/(country_id+".json");
            write_json(ai_path,{{"country_id",country_id},{"ai_runtime",ai_values}});
            shard_paths.push_back(ai_path);
        }
        json promoted=json::array();
        for (const auto& [id,value] : impl_->promoted) {
            (void)id;
            promoted.push_back({{"npc_id",value.npc_id},{"country_id",value.country_id},
                                {"population_class",value.population_class},{"population_index",value.population_index},
                                {"deterministic_seed",value.deterministic_seed},{"occupation_fixture",value.occupation_fixture},
                                {"disposition_fixture",value.disposition_fixture},{"promoted",true},
                                {"important_event_ids",value.important_event_ids}});
        }
        write_json(temporary/"promoted_non_ai.json",{{"promoted",promoted}});
        shard_paths.push_back(temporary/"promoted_non_ai.json");
        json pending=json::array();
        for (const auto& event : impl_->pending) pending.push_back(impl_->event_to_json(event));
        write_json(temporary/"pending_events.json",{{"pending",pending}});
        shard_paths.push_back(temporary/"pending_events.json");
        write_text(temporary/"visible_core.json",visible_core_save_json.back()=='\n'?visible_core_save_json:visible_core_save_json+"\n");
        shard_paths.push_back(temporary/"visible_core.json");
        write_json(temporary/"timestamp.json",{{"saved_at_utc_epoch",saved_at_utc_epoch}});
        shard_paths.push_back(temporary/"timestamp.json");
        impl_->flush_log_segment();
        json segment_index=json::array();
        for (const auto& segment : impl_->segments) {
            const auto segment_relative=std::filesystem::path("log_segments")/(segment.segment_id+".jsonl");
            const auto segment_path=temporary/segment_relative;
            if (segment.path.empty()) {
                if (sha256_impl(segment.in_memory)!=segment.sha256)
                    throw std::runtime_error("audit segment source hash mismatch: "+segment.segment_id);
                write_text(segment_path,segment.in_memory);
            } else {
                if (sha256_file_impl(segment.path)!=segment.sha256)
                    throw std::runtime_error("audit segment source hash mismatch: "+segment.segment_id);
                std::filesystem::create_directories(segment_path.parent_path());
                std::filesystem::copy_file(segment.path,segment_path,std::filesystem::copy_options::none);
            }
            shard_paths.push_back(segment_path);
            segment_index.push_back({{"segment_id",segment.segment_id},{"country_id",segment.country_id},
                                     {"first_event_id",segment.first_event_id},{"last_event_id",segment.last_event_id},
                                     {"first_tick",segment.first_tick},{"last_tick",segment.last_tick},
                                     {"entry_count",segment.entry_count},{"segment_sha256",segment.sha256},
                                     {"path",segment_relative.generic_string()}});
        }
        write_json(temporary/"audit_segments.json",{{"segments",segment_index}});
        shard_paths.push_back(temporary/"audit_segments.json");
        if (failure_injection=="BEFORE_MANIFEST_WRITE") throw std::runtime_error("injected BEFORE_MANIFEST_WRITE");

        json shards=json::array();
        for (const auto& path : shard_paths) shards.push_back({{"path",std::filesystem::relative(path,temporary).generic_string()},
                                                               {"sha256",sha256_file_impl(path)}});
        json generation_manifest{{"generation_id",result.generation_id},{"parent_generation_id",result.parent_generation_id},
                                 {"created_at",impl_->world_time},{"simulation_version","stage-f-0.1.0"},
                                 {"schema_version","stage_f_save_generation_v1"},{"scale_data_sha256",impl_->scale_hash},
                                 {"shards",shards}};
        if (failure_injection=="DURING_MANIFEST_WRITE") {
            write_text(temporary/"generation_manifest.json","{\n");
            throw std::runtime_error("injected DURING_MANIFEST_WRITE");
        }
        write_json(temporary/"generation_manifest.json",generation_manifest);
        result.manifest_sha256=sha256_file_impl(temporary/"generation_manifest.json");
        for (const auto& shard : shards)
            if (sha256_file_impl(temporary / shard.at("path").get<std::string>()) !=
                shard.at("sha256").get<std::string>())
                throw std::runtime_error("generation shard readback mismatch");
        if (failure_injection=="BEFORE_CURRENT_SWITCH") throw std::runtime_error("injected BEFORE_CURRENT_SWITCH");
        std::filesystem::rename(temporary,final);

        json top{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_save_schema_v1"},
                 {"simulation_version","stage-f-0.1.0"},{"generation_id",result.generation_id},
                 {"parent_generation_id",result.parent_generation_id},{"created_at",impl_->world_time},
                 {"manifest_hash",result.manifest_sha256},{"generation_manifest_path",
                    std::filesystem::relative(final/"generation_manifest.json",save_manifest_path.parent_path()).generic_string()},
                 {"scale_data_sha256",impl_->scale_hash},{"migration",impl_->migration}};
        const std::string old_top=std::filesystem::exists(save_manifest_path)?read_text(save_manifest_path):"";
        const auto next_top=save_manifest_path.string()+".next";
        write_json(next_top,top);
        const auto parsed_top = json::parse(read_text(next_top));
        (void)parsed_top;
        if (!old_top.empty()) {
            const std::string old_sha=sha256_impl(old_top);
            const std::string backup_label=result.parent_generation_id.empty()?"pre-stage-f":result.parent_generation_id;
            const auto backup=save_manifest_path.string()+"."+backup_label+"-"+old_sha+".bak";
            std::string backup_error;
            if (!immutable_copy(save_manifest_path,backup,old_sha,backup_error)) throw std::runtime_error(backup_error);
        }
        const auto switching=save_manifest_path.string()+".switch";
        if (!old_top.empty()) std::filesystem::rename(save_manifest_path,switching);
        try { std::filesystem::rename(next_top,save_manifest_path); }
        catch (...) {
            if (!old_top.empty()) std::filesystem::rename(switching,save_manifest_path);
            throw;
        }
        if (failure_injection=="AFTER_SWITCH_READBACK_FAILURE") {
            std::filesystem::remove(save_manifest_path);
            if (!old_top.empty()) std::filesystem::rename(switching,save_manifest_path);
            const auto rejected=final.string()+".rejected.tmp";
            if (std::filesystem::exists(final)) std::filesystem::rename(final,rejected);
            throw std::runtime_error("injected AFTER_SWITCH_READBACK_FAILURE");
        }
        const json checked=read_json(save_manifest_path);
        if (checked.at("manifest_hash")!=result.manifest_sha256) throw std::runtime_error("current generation readback mismatch");
        if (!old_top.empty() && std::filesystem::exists(switching)) std::filesystem::remove(switching);
        impl_->save_generation=next;
        impl_->refresh_counters();
        result.success=true;
        result.audit_message="STAGE_F_SAVE_GENERATION_COMMITTED";
    } catch (const std::exception& exception) {
        result.error=exception.what();
        result.audit_message="STAGE_F_SAVE_GENERATION_REJECTED";
    }
    impl_->counters.last_save_duration_milliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count();
    return result;
}

namespace {
bool validate_generation(const std::filesystem::path& manifest_path, std::string& error) {
    try {
        const json manifest=read_json(manifest_path);
        if (manifest.value("schema_version","")!="stage_f_save_generation_v1") throw std::runtime_error("generation schema mismatch");
        for (const auto& shard : manifest.at("shards")) {
            const auto path=manifest_path.parent_path()/shard.at("path").get<std::string>();
            if (!std::filesystem::exists(path) ||
                sha256_file_impl(path) != shard.at("sha256").get<std::string>())
                throw std::runtime_error("generation shard hash mismatch: "+path.string());
        }
        return true;
    } catch (const std::exception& exception) { error=exception.what(); return false; }
}
}

StageFProductionRuntime StageFProductionRuntime::load_generation(const std::filesystem::path& scale_data_root,
                                                                 const std::filesystem::path& save_manifest_path,
                                                                 std::filesystem::path& out_visible_core_save,
                                                                 StageFSaveResult& out_result,
                                                                 const std::filesystem::path& log_archive_root) {
    const auto started=std::chrono::steady_clock::now();
    StageFProductionRuntime runtime=load(scale_data_root,log_archive_root);
    const json top=read_json(save_manifest_path);
    if (top.value("schema_version","")!="stage_f_save_schema_v1" || top.value("simulation_version","")!="stage-f-0.1.0")
        throw std::runtime_error("not a Stage F save manifest");
    const auto generation_root=save_manifest_path.parent_path()/"stage_f_generations";
    std::vector<std::filesystem::path> candidates;
    const auto current=save_manifest_path.parent_path()/top.at("generation_manifest_path").get<std::string>();
    candidates.push_back(current);
    for (const auto& entry : std::filesystem::directory_iterator(generation_root))
        if (entry.is_directory() && !entry.path().filename().string().ends_with(".tmp")) {
            const auto candidate=entry.path()/"generation_manifest.json";
            if (candidate!=current) candidates.push_back(candidate);
        }
    std::sort(candidates.begin()+1,candidates.end(),std::greater<>());
    std::filesystem::path selected;
    std::string validation_error;
    for (const auto& candidate : candidates) {
        if (!validate_generation(candidate,validation_error)) continue;
        if (candidate==current && sha256_file_impl(candidate)!=top.value("manifest_hash","")) {
            validation_error="current generation manifest hash mismatch";
            continue;
        }
        selected=candidate;
        break;
    }
    if (selected.empty()) throw std::runtime_error("no valid Stage F save generation: "+validation_error);
    out_result.recovered_previous_generation=selected!=current;
    const json generation=read_json(selected);
    if (generation.value("scale_data_sha256","")!=runtime.scale_data_sha256())
        throw std::runtime_error("Stage F save generation scale data hash mismatch");
    out_result.generation_id=generation.at("generation_id");
    out_result.parent_generation_id=generation.value("parent_generation_id","");
    out_result.manifest_sha256=sha256_file_impl(selected);
    out_result.success=true;
    out_result.audit_message=out_result.recovered_previous_generation?"STAGE_F_RECOVERED_PREVIOUS_GENERATION":"STAGE_F_GENERATION_LOADED";
    auto& impl=*runtime.impl_;
    const json world=read_json(selected.parent_path()/"world.json");
    impl.world_time=world.at("world_time"); impl.next_event_sequence=world.at("next_event_sequence");
    impl.simulation_tick=world.at("simulation_tick"); impl.population_interval_count=world.at("population_interval_count");
    impl.migration=world.value("migration",json::object());
    impl.location_partition.clear(); impl.active_set.clear(); impl.due_queue.clear();
    impl.active_count=0; impl.background_count=0; impl.dormant_count=0;
    std::array<bool,2500> loaded_ai{};
    for (int country_index=1;country_index<=5;++country_index) {
        const std::string country_id="country_"+padded(country_index,3);
        const json value=read_json(selected.parent_path()/"countries"/(country_id+".json"));
        const auto country=impl.countries.find(country_id);
        if (country==impl.countries.end() || value.value("country_id","")!=country_id)
            throw std::runtime_error("Stage F save country partition mismatch: "+country_id);
        country->second.local_events=value.at("local_events");
        country->second.cross_country_events_received=value.at("cross_country_events_received");
        country->second.interval_count=value.at("interval_count");
        country->second.last_root_event_id=value.value("last_root_event_id","");
        const json ai_shard=read_json(selected.parent_path()/"ai"/(country_id+".json"));
        if (ai_shard.value("country_id","")!=country_id || ai_shard.at("ai_runtime").size()!=500)
            throw std::runtime_error("Stage F save AI shard partition/count mismatch: "+country_id);
        for (const auto& ai_value : ai_shard.at("ai_runtime")) {
            auto npc=impl.ai_from_json(ai_value); const std::string id=npc.npc_id;
            if (!id.starts_with("ai_npc_")) throw std::runtime_error("Stage F save AI id invalid: "+id);
            const int ordinal=std::stoi(id.substr(7));
            if (ordinal<1 || ordinal>2500 || id!="ai_npc_"+padded(ordinal,3) || loaded_ai[static_cast<std::size_t>(ordinal-1)])
                throw std::runtime_error("Stage F save AI id duplicate/out of range: "+id);
            loaded_ai[static_cast<std::size_t>(ordinal-1)]=true;
            const auto existing=impl.ai.find(id);
            if (existing==impl.ai.end() || npc.country_id!=country_id)
                throw std::runtime_error("Stage F save AI country partition mismatch: "+id);
            existing->second=std::move(npc);
            const auto& restored=existing->second;
            impl.location_partition[restored.current_location_id].insert(id);
            if (restored.activity_tier==StageFActivityTier::active) impl.active_set.insert(id);
            if (restored.activity_tier==StageFActivityTier::background && restored.next_due_time)
                impl.due_queue.insert({*restored.next_due_time,restored.country_id,id});
            switch (restored.activity_tier) {
            case StageFActivityTier::active: ++impl.active_count; break;
            case StageFActivityTier::background: ++impl.background_count; break;
            case StageFActivityTier::dormant: ++impl.dormant_count; break;
            }
        }
    }
    if (std::find(loaded_ai.begin(),loaded_ai.end(),false)!=loaded_ai.end())
        throw std::runtime_error("Stage F save AI runtime is incomplete");
    impl.promoted.clear();
    for (const auto& value : read_json(selected.parent_path()/"promoted_non_ai.json").at("promoted")) {
        StageFMaterializedNonAi npc;
        npc.npc_id=value.at("npc_id"); npc.country_id=value.at("country_id"); npc.population_class=value.at("population_class");
        npc.population_index=value.at("population_index"); npc.deterministic_seed=value.at("deterministic_seed");
        npc.occupation_fixture=value.at("occupation_fixture"); npc.disposition_fixture=value.at("disposition_fixture");
        npc.promoted=true; npc.important_event_ids=value.at("important_event_ids").get<std::vector<std::string>>();
        impl.promoted.emplace(npc.npc_id,npc);
    }
    impl.pending.clear();
    for (const auto& value : read_json(selected.parent_path()/"pending_events.json").at("pending"))
        impl.pending.insert(impl.event_from_json(value));
    impl.segments.clear();
    impl.hot_log.clear();
    for (const auto& value : read_json(selected.parent_path()/"audit_segments.json").at("segments")) {
        Impl::Segment segment;
        segment.segment_id=value.at("segment_id"); segment.country_id=value.at("country_id");
        segment.first_event_id=value.at("first_event_id"); segment.last_event_id=value.at("last_event_id");
        segment.first_tick=value.at("first_tick"); segment.last_tick=value.at("last_tick");
        segment.entry_count=value.at("entry_count"); segment.sha256=value.at("segment_sha256");
        segment.path=selected.parent_path()/value.at("path").get<std::string>();
        if (sha256_file_impl(segment.path)!=segment.sha256)
            throw std::runtime_error("Stage F audit segment hash mismatch: "+segment.segment_id);
        impl.segments.push_back(std::move(segment));
    }
    impl.save_generation=std::stoull(out_result.generation_id.substr(4));
    out_visible_core_save=selected.parent_path()/"visible_core.json";
    std::size_t incomplete=0;
    for (const auto& entry : std::filesystem::directory_iterator(generation_root))
        if (entry.is_directory() && entry.path().filename().string().ends_with(".tmp")) ++incomplete;
    if (incomplete) out_result.audit_message+=";INCOMPLETE_GENERATIONS="+std::to_string(incomplete);
    impl.refresh_counters();
    impl.counters.last_load_duration_milliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count();
    return runtime;
}

StageFMigrationResult StageFProductionRuntime::migrate_stage_e_save(
    const std::filesystem::path& scale_data_root, const std::filesystem::path& save_manifest_path,
    const std::filesystem::path& timestamp_metadata_path, std::int64_t saved_at_utc_epoch,
    const std::filesystem::path& log_archive_root) {
    StageFMigrationResult result;
    try {
        const std::string source=read_text(save_manifest_path);
        const json old=json::parse(source);
        if (old.value("schema_version","")=="stage_f_save_schema_v1") { result.success=true; return result; }
        if (old.value("schema_version","")!="stage_e_save_schema_v1") throw std::runtime_error("Stage F migration source is not Stage E");
        if (!std::filesystem::exists(timestamp_metadata_path)) throw std::runtime_error("Stage E timestamp metadata missing");
        result.source_save_sha256=sha256_impl(source);
        result.source_metadata_sha256=sha256_file_impl(timestamp_metadata_path);
        result.save_backup_path=save_manifest_path.string()+".stage-e-"+result.source_save_sha256+".bak";
        result.metadata_backup_path=timestamp_metadata_path.string()+".stage-e-"+result.source_save_sha256+".bak";
        if (!immutable_copy(save_manifest_path,result.save_backup_path,result.source_save_sha256,result.error) ||
            !immutable_copy(timestamp_metadata_path,result.metadata_backup_path,result.source_metadata_sha256,result.error)) return result;
        auto runtime=load(scale_data_root,log_archive_root);
        for (const auto& value : old.at("ai_runtime")) {
            const std::string id=value.at("npc_id");
            const auto found=runtime.impl_->ai.find(id);
            if (found==runtime.impl_->ai.end()) continue;
            auto& npc=found->second;
            npc.current_state_id=value.at("current_state_id"); npc.player_evaluation=value.value("player_evaluation",0);
            npc.current_goal=value.value("current_goal",npc.current_goal);
            npc.relationships_summary=value.value("relationships",decltype(npc.relationships_summary){});
            npc.known_event_summary=value.value("known_events",std::vector<std::string>{});
            npc.active_action=value.value("active_action","");
        }
        runtime.impl_->world_time=old.at("world").value("current_world_time_minutes",std::int64_t{});
        runtime.impl_->migration={{"migration_id","stage_e_to_stage_f_v1"},
                                  {"source_schema_version",old.value("schema_version","")},
                                  {"source_simulation_version",old.value("simulation_version","")},
                                  {"source_save_sha256",result.source_save_sha256},
                                  {"source_metadata_sha256",result.source_metadata_sha256},
                                  {"source_backup_path",result.save_backup_path.generic_string()},
                                  {"metadata_backup_path",result.metadata_backup_path.generic_string()}};
        const auto saved=runtime.save_generation(save_manifest_path,source,saved_at_utc_epoch);
        if (!saved.success) throw std::runtime_error(saved.error);
        result.success=true; result.migrated=true;
    } catch (const std::exception& exception) { result.error=exception.what(); }
    return result;
}

std::string StageFProductionRuntime::canonical_snapshot() const {
    json countries=json::array();
    for (const auto& [id,value] : impl_->countries)
        countries.push_back({{"country_id",id},{"local_events",value.local_events},
                             {"cross_country_events_received",value.cross_country_events_received},
                             {"interval_count",value.interval_count},{"last_root_event_id",value.last_root_event_id}});
    json ai=json::array();
    for (const auto& [id,value] : impl_->ai) { (void)id; ai.push_back(impl_->ai_to_json(value)); }
    json promoted=json::array();
    for (const auto& [id,value] : impl_->promoted) {
        (void)id;
        promoted.push_back({{"npc_id",value.npc_id},{"country_id",value.country_id},
                            {"population_class",value.population_class},{"population_index",value.population_index},
                            {"deterministic_seed",value.deterministic_seed},{"important_event_ids",value.important_event_ids}});
    }
    json pending=json::array();
    for (const auto& value : impl_->pending) pending.push_back(impl_->event_to_json(value));
    json segments=json::array();
    for (const auto& value : impl_->segments) segments.push_back({{"segment_id",value.segment_id},{"sha256",value.sha256},{"entry_count",value.entry_count}});
    return json{{"schema_version","stage_f_canonical_snapshot_v1"},{"simulation_version","stage-f-0.1.0"},
                {"scale_data_sha256",impl_->scale_hash},{"world_seed",impl_->world_seed},{"world_time",impl_->world_time},
                {"simulation_tick",impl_->simulation_tick},{"next_event_sequence",impl_->next_event_sequence},
                {"save_generation",impl_->save_generation},{"countries",countries},{"ai_runtime",ai},
                {"promoted_non_ai",promoted},{"pending_events",pending},{"population_interval_count",impl_->population_interval_count},
                {"log_segments",segments},{"migration",impl_->migration}}.dump();
}

std::string StageFProductionRuntime::canonical_sha256() const { return sha256_impl(canonical_snapshot()); }

std::vector<std::string> StageFProductionRuntime::archived_causal_path(const std::string& root_event_id) const {
    std::vector<std::string> result;
    for (const auto& segment : impl_->segments) {
        const std::string rows=segment.path.empty()?segment.in_memory:read_text(segment.path);
        std::istringstream input(rows); std::string line;
        while (std::getline(input,line)) if (!line.empty()) {
            const json value=json::parse(line);
            if (value.value("root_event_id","")==root_event_id) result.push_back(value.value("event_id","")+":"+value.value("event_type",""));
        }
    }
    for (const auto& value : impl_->hot_log)
        if (value.value("root_event_id","")==root_event_id) result.push_back(value.value("event_id","")+":"+value.value("event_type",""));
    return result;
}

void StageFProductionRuntime::set_all_active_reference_mode(bool enabled) { impl_->all_active_reference_mode=enabled; }
std::string StageFProductionRuntime::sha256_bytes(const std::string& bytes) { return sha256_impl(bytes); }
std::string StageFProductionRuntime::sha256_file(const std::filesystem::path& path) { return sha256_file_impl(path); }

} // namespace nation_sim
