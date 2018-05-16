#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include "primitives.hpp"
#include "geometry.hpp"
#include "hashtable.hpp"
#include "bitfield.hpp"

// The structs defined in this file should be copyable, pointerless blobs of data.

struct StatusEffect {
    enum Id {
        CONFUSION,
        SPEED,
        ETHEREAL_VISION,
        COGNISCOPY,
        BLINDNESS,
        INVISIBILITY,
        POISON,
        POLYMORPH,
        SLOWING,
        BURROWING,
        LEVITATING,
        PUSHED,

        COUNT,
    };
    Id type;
    // this is never in the past
    int64_t expiration_time;

    int64_t poison_next_damage_time;
    // used for awarding experience
    uint256 who_is_responsible;
    // used for polymorph
    SpeciesId species_id;
    // used for levitation momentum
    Coord coord;
};

using StatusEffectIdBitField = BitField<StatusEffect::COUNT - 1>::Type;

struct Event {
    enum Type {
        THE_INDIVIDUAL,
        THE_LOCATION,
        INDIVIDUAL_AND_STATUS,
        INDIVIDUAL_AND_LOCATION,
        MOVE,
        TWO_INDIVIDUAL,
        INDIVIDUAL_AND_ITEM,
        POLYMORPH,
        ITEM_AND_LOCATION,
    };
    Type type;

    struct IndividualAndLocationData {
        enum Id {
            BUMP_INTO_LOCATION,
            ATTACK_LOCATION,
            INDIVIDUAL_BURROWS_THROUGH_WALL,
        };
        Id id;
        uint256 actor;
        Coord location;
        bool is_air;
    };
    struct MoveData {
        uint256 actor;
        Coord old_location;
        Coord new_location;
    };
    struct TwoIndividualData {
        enum Id {
            BUMP_INTO_INDIVIDUAL,
            ATTACK_INDIVIDUAL,
            DODGE_ATTACK,
            MELEE_KILL,
        };
        Id id;
        uint256 actor;
        uint256 target;
    };
    struct IndividualAndItemData {
        enum Id {
            ZAP_WAND,
            ZAP_WAND_NO_CHARGES,
            WAND_DISINTEGRATES,
            READ_BOOK,
            THROW_ITEM,
            ITEM_HITS_INDIVIDUAL,
            ITEM_SINKS_INTO_INDIVIDUAL,
            INDIVIDUAL_DODGES_THROWN_ITEM,
            INDIVIDUAL_PICKS_UP_ITEM,
            INDIVIDUAL_SUCKS_UP_ITEM,
            QUAFF_POTION,
            POTION_HITS_INDIVIDUAL,
        };
        Id id;
        uint256 individual;
        uint256 item;
    };
    struct ItemAndLocationData {
        enum Id {
            WAND_EXPLODES,
            ITEM_HITS_WALL,
            ITEM_DROPS_TO_THE_FLOOR,
            POTION_BREAKS,
            ITEM_DISINTEGRATES_IN_LAVA,
        };
        Id id;
        uint256 item;
        Coord location;
    };
    struct TheIndividualData {
        enum Id {
            APPEAR,
            LEVEL_UP,
            DIE,
            DELETE_THING,
            SPIT_BLINDING_VENOM,
            BLINDING_VENOM_HIT_INDIVIDUAL,
            THROW_TAR,
            TAR_HIT_INDIVIDUAL,
            MAGIC_BEAM_HIT_INDIVIDUAL,
            INDIVIDUAL_DODGES_MAGIC_BEAM,
            MAGIC_MISSILE_HIT_INDIVIDUAL,
            MAGIC_BULLET_HIT_INDIVIDUAL,
            INDIVIDUAL_IS_HEALED,
            ACTIVATED_MAPPING,
            MAGIC_BEAM_PUSH_INDIVIDUAL,
            MAGIC_BEAM_RECOILS_AND_PUSHES_INDIVIDUAL,
            FAIL_TO_CAST_SPELL,
            SEARED_BY_LAVA,
            INDIVIDUAL_FLOATS_UNCONTROLLABLY,
            LUNGE,
        };
        Id id;
        uint256 individual;
    };
    struct TheLocationData {
        enum Id {
            MAGIC_BEAM_HIT_WALL,
            BEAM_OF_DIGGING_DIGS_WALL,
            MAGIC_BEAM_PASS_THROUGH_AIR,
        };
        Id id;
        Coord location;
    };
    struct IndividualAndStatusData {
        bool is_gain; // otherwise lose
        uint256 individual;
        StatusEffect::Id status;
    };

    TheIndividualData & the_individual_data() {
        check_data_type(THE_INDIVIDUAL);
        return _data._the_individual;
    }

    TheLocationData & the_location_data() {
        check_data_type(THE_LOCATION);
        return _data._the_location;
    }

    IndividualAndStatusData & individual_and_status_data() {
        check_data_type(INDIVIDUAL_AND_STATUS);
        return _data._individual_and_status;
    }

    IndividualAndLocationData & individual_and_location_data() {
        check_data_type(INDIVIDUAL_AND_LOCATION);
        return _data._individual_and_location;
    }

    MoveData & move_data() {
        check_data_type(MOVE);
        return _data._move;
    }

    TwoIndividualData & two_individual_data() {
        check_data_type(TWO_INDIVIDUAL);
        return _data._two_individual;
    }

    IndividualAndItemData & individual_and_item_data() {
        check_data_type(INDIVIDUAL_AND_ITEM);
        return _data._individual_and_item;
    }

    struct PolymorphData {
        uint256 individual;
        SpeciesId new_species;
    };
    PolymorphData & polymorph_data() {
        check_data_type(POLYMORPH);
        return _data._polymorph;
    };

    ItemAndLocationData & item_and_location_data() {
        check_data_type(ITEM_AND_LOCATION);
        return _data._item_and_location;
    }

    static inline Event move(uint256 mover, Coord from, Coord to) {
        return move_event(mover, from, to);
    }

    static inline Event attack_individual(uint256 attacker, uint256 target) {
        return two_individual_type_event(TwoIndividualData::ATTACK_INDIVIDUAL, attacker, target);
    }
    static inline Event dodge_attack(uint256 attacker, uint256 target) {
        return two_individual_type_event(TwoIndividualData::DODGE_ATTACK, attacker, target);
    }
    static inline Event attack_location(uint256 attacker, Coord location, bool is_air) {
        return individual_and_location_event(IndividualAndLocationData::ATTACK_LOCATION, attacker, location, is_air);
    }

    static inline Event zap_wand(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::ZAP_WAND, individual_id, item_id);
    }
    static inline Event wand_zap_no_charges(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::ZAP_WAND_NO_CHARGES, individual_id, item_id);
    }
    static inline Event wand_disintegrates(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::WAND_DISINTEGRATES, individual_id, item_id);
    }
    static inline Event wand_explodes(uint256 item_id, Coord location) {
        return item_and_location_type_event(ItemAndLocationData::WAND_EXPLODES, item_id, location);
    }
    static inline Event read_book(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::READ_BOOK, individual_id, item_id);
    }
    static inline Event magic_beam_hit(uint256 target) {
        return individual_event(TheIndividualData::MAGIC_BEAM_HIT_INDIVIDUAL, target);
    }
    static inline Event individual_dodges_magic_beam(uint256 target) {
        return individual_event(TheIndividualData::INDIVIDUAL_DODGES_MAGIC_BEAM, target);
    }
    static inline Event magic_missile_hit(uint256 target) {
        return individual_event(TheIndividualData::MAGIC_MISSILE_HIT_INDIVIDUAL, target);
    }
    static inline Event magic_bullet_hit(uint256 target) {
        return individual_event(TheIndividualData::MAGIC_BULLET_HIT_INDIVIDUAL, target);
    }
    static inline Event magic_beam_hit_wall(Coord location) {
        return location_event(TheLocationData::MAGIC_BEAM_HIT_WALL, location);
    }
    static inline Event beam_of_digging_digs_wall(Coord location) {
        return location_event(TheLocationData::BEAM_OF_DIGGING_DIGS_WALL, location);
    }
    static inline Event magic_beam_pass_through_air(Coord location) {
        return location_event(TheLocationData::MAGIC_BEAM_PASS_THROUGH_AIR, location);
    }
    static inline Event individual_is_healed(uint256 target) {
        return individual_event(TheIndividualData::INDIVIDUAL_IS_HEALED, target);
    }

    static Event bump_into_individual(uint256 actor, uint256 target) {
        return two_individual_type_event(TwoIndividualData::BUMP_INTO_INDIVIDUAL, actor, target);
    }
    static Event bump_into_location(uint256 actor, Coord location, bool is_air) {
        return individual_and_location_event(IndividualAndLocationData::BUMP_INTO_LOCATION, actor, location, is_air);
    }
    static Event individual_burrows_through_wall(uint256 actor_id, Coord location) {
        return individual_and_location_event(IndividualAndLocationData::INDIVIDUAL_BURROWS_THROUGH_WALL, actor_id, location, true);
    }

    static Event throw_item(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::THROW_ITEM, individual_id, item_id);
    }
    static Event item_hits_individual(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::ITEM_HITS_INDIVIDUAL, individual_id, item_id);
    }
    static Event item_sinks_into_individual(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::ITEM_SINKS_INTO_INDIVIDUAL, individual_id, item_id);
    }
    static Event individual_dodges_thrown_item(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::INDIVIDUAL_DODGES_THROWN_ITEM, individual_id, item_id);
    }
    static Event item_hits_wall(uint256 item_id, Coord location) {
        return item_and_location_type_event(ItemAndLocationData::ITEM_HITS_WALL, item_id, location);
    }
    static Event potion_breaks(uint256 item_id, Coord location) {
        return item_and_location_type_event(ItemAndLocationData::POTION_BREAKS, item_id, location);
    }
    static Event item_disintegrates_in_lava(uint256 item_id, Coord location) {
        return item_and_location_type_event(ItemAndLocationData::ITEM_DISINTEGRATES_IN_LAVA, item_id, location);
    }

    static inline Event gain_status(uint256 individual_id, StatusEffect::Id status) {
        return individual_and_status_event(true, individual_id, status);
    }
    static inline Event lose_status(uint256 individual_id, StatusEffect::Id status) {
        return individual_and_status_event(false, individual_id, status);
    }

    static inline Event spit_blinding_venom(uint256 actor_id) {
        return individual_event(TheIndividualData::SPIT_BLINDING_VENOM, actor_id);
    }
    static inline Event blinding_venom_hit_individual(uint256 target_id) {
        return individual_event(TheIndividualData::BLINDING_VENOM_HIT_INDIVIDUAL, target_id);
    }
    static inline Event throw_tar(uint256 actor_id) {
        return individual_event(TheIndividualData::THROW_TAR, actor_id);
    }
    static inline Event tar_hit_individual(uint256 target_id) {
        return individual_event(TheIndividualData::TAR_HIT_INDIVIDUAL, target_id);
    }

    static inline Event appear(uint256 new_guy) {
        return individual_event(TheIndividualData::APPEAR, new_guy);
    }
    static inline Event level_up(uint256 individual_id) {
        return individual_event(TheIndividualData::LEVEL_UP, individual_id);
    }
    static inline Event die(uint256 deceased_id) {
        return individual_event(TheIndividualData::DIE, deceased_id);
    }
    static inline Event delete_thing(uint256 deceased_id) {
        return individual_event(TheIndividualData::DELETE_THING, deceased_id);
    }
    static inline Event melee_kill(uint256 attacker, uint256 deceased) {
        return two_individual_type_event(TwoIndividualData::MELEE_KILL, attacker, deceased);
    }
    static inline Event activated_mapping(uint256 individual_id) {
        return individual_event(TheIndividualData::ACTIVATED_MAPPING, individual_id);
    }
    static inline Event magic_beam_push_individual(uint256 individual_id) {
        return individual_event(TheIndividualData::MAGIC_BEAM_PUSH_INDIVIDUAL, individual_id);
    }
    static inline Event magic_beam_recoils_and_pushes_individual(uint256 individual_id) {
        return individual_event(TheIndividualData::MAGIC_BEAM_RECOILS_AND_PUSHES_INDIVIDUAL, individual_id);
    }
    static inline Event fail_to_cast_spell(uint256 individual_id) {
        return individual_event(TheIndividualData::FAIL_TO_CAST_SPELL, individual_id);
    }
    static inline Event seared_by_lava(uint256 individual_id) {
        return individual_event(TheIndividualData::SEARED_BY_LAVA, individual_id);
    }
    static inline Event individual_floats_uncontrollably(uint256 individual_id) {
        return individual_event(TheIndividualData::INDIVIDUAL_FLOATS_UNCONTROLLABLY, individual_id);
    }
    static inline Event lunge(uint256 individual_id) {
        return individual_event(TheIndividualData::LUNGE, individual_id);
    }

    static inline Event polymorph(uint256 shapeshifter, SpeciesId new_species) {
        Event result;
        result.type = POLYMORPH;
        PolymorphData & data = result.polymorph_data();
        data.individual = shapeshifter;
        data.new_species = new_species;
        return result;
    }


    static inline Event item_drops_to_the_floor(uint256 item, Coord location) {
        return item_and_location_type_event(ItemAndLocationData::ITEM_DROPS_TO_THE_FLOOR, item, location);
    }
    static inline Event individual_picks_up_item(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::INDIVIDUAL_PICKS_UP_ITEM, individual_id, item_id);
    }
    static inline Event individual_sucks_up_item(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::INDIVIDUAL_SUCKS_UP_ITEM, individual_id, item_id);
    }
    static inline Event quaff_potion(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::QUAFF_POTION, individual_id, item_id);
    }
    static inline Event potion_hits_individual(uint256 individual_id, uint256 item_id) {
        return individual_and_item_event(IndividualAndItemData::POTION_HITS_INDIVIDUAL, individual_id, item_id);
    }

private:
    static inline Event individual_event(TheIndividualData::Id id, uint256 individual_id) {
        Event result;
        result.type = THE_INDIVIDUAL;
        result.the_individual_data() = {
            id,
            individual_id,
        };
        return result;
    }
    static inline Event location_event(TheLocationData::Id id, Coord location) {
        Event result;
        result.type = THE_LOCATION;
        result.the_location_data() = {
            id,
            location,
        };
        return result;
    }
    static inline Event individual_and_status_event(bool is_gain, uint256 individual_id, StatusEffect::Id status) {
        Event result;
        result.type = INDIVIDUAL_AND_STATUS;
        result.individual_and_status_data() = {
            is_gain,
            individual_id,
            status,
        };
        return result;
    }
    static inline Event individual_and_location_event(IndividualAndLocationData::Id id, uint256 actor, Coord location, bool is_air) {
        Event result;
        result.type = INDIVIDUAL_AND_LOCATION;
        result.individual_and_location_data() = {
            id,
            actor,
            location,
            is_air,
        };
        return result;
    }
    static inline Event move_event(uint256 actor, Coord old_location, Coord new_location) {
        Event result;
        result.type = MOVE;
        result.move_data() = {
            actor,
            old_location,
            new_location,
        };
        return result;
    }
    static inline Event two_individual_type_event(TwoIndividualData::Id id, uint256 actor, uint256 target) {
        Event result;
        result.type = TWO_INDIVIDUAL;
        result.two_individual_data() = {
            id,
            actor,
            target,
        };
        return result;
    }
    static inline Event individual_and_item_event(IndividualAndItemData::Id id, uint256 individual_id, uint256 item_id) {
        Event result;
        result.type = INDIVIDUAL_AND_ITEM;
        result.individual_and_item_data() = {
            id,
            individual_id,
            item_id,
        };
        return result;
    }
    static inline Event item_and_location_type_event(ItemAndLocationData::Id id, uint256 item, Coord location) {
        Event result;
        result.type = ITEM_AND_LOCATION;
        result.item_and_location_data() = {
            id,
            item,
            location,
        };
        return result;
    }

    union {
        TheIndividualData _the_individual;
        TheLocationData _the_location;
        IndividualAndStatusData _individual_and_status;
        IndividualAndLocationData _individual_and_location;
        MoveData _move;
        TwoIndividualData _two_individual;
        IndividualAndItemData _individual_and_item;
        PolymorphData _polymorph;
        ItemAndLocationData _item_and_location;
    } _data;

    void check_data_type(Type supposed_data_type) const {
        assert_str(supposed_data_type == type, "wrong data type");
    }
};

struct PerceivedEvent {
};

// everyone has the same action cost
static const int action_cost = 12;
static const int speedy_movement_cost = 3;
static const int slow_movement_cost = 48;

struct Species {
    SpeciesId species_id;
    // how many ticks does it cost to move one space? average human is 12.
    int movement_cost;
    int base_hitpoints;
    int base_mana;
    int base_attack_power;
    int min_level;
    int max_level;
    Mind mind;
    VisionTypes vision_types;
    bool sucks_up_items;
    bool auto_throws_items;
    bool poison_attack;
    bool flying;
};
static const Mind _none = Mind_NONE;
static const Mind _beas = Mind_BEAST;
static const Mind _savg = Mind_SAVAGE;
static const Mind _civl = Mind_CIVILIZED;
static const VisionTypes _norm = VisionTypes_NORMAL;
static const VisionTypes _ethe = VisionTypes_ETHEREAL;
static constexpr Species specieses[SpeciesId_COUNT] = {
    //                         movement cost
    //                         |   health
    //                         |   |  base mana
    //                         |   |  |  base attack
    //                         |   |  |  |  min level
    //                         |   |  |  |  |   max level
    //                         |   |  |  |  |   |  mind
    //                         |   |  |  |  |   |  |     vision
    //                         |   |  |  |  |   |  |     |     sucks up items
    //                         |   |  |  |  |   |  |     |     |  auto throws items
    //                         |   |  |  |  |   |  |     |     |  |  poison attack
    //                         |   |  |  |  |   |  |     |     |  |  |  flying
    {SpeciesId_HUMAN        , 12, 10, 3, 3, 0, 10,_civl,_norm, 0, 0, 0, 0},
    {SpeciesId_OGRE         , 24, 15, 0, 2, 4, 10,_savg,_norm, 0, 0, 0, 0},
    {SpeciesId_LICH         , 12, 12, 4, 3, 7, 10,_civl,_norm, 0, 0, 0, 0},
    {SpeciesId_SHAPESHIFTER , 12,  5, 0, 2, 1, 10,_civl,_norm, 0, 0, 0, 0},
    {SpeciesId_PINK_BLOB    , 48,  4, 0, 1, 0,  1,_none,_ethe, 1, 0, 0, 0},
    {SpeciesId_AIR_ELEMENTAL,  6,  6, 0, 1, 3, 10,_none,_ethe, 1, 1, 0, 1},
    {SpeciesId_TAR_ELEMENTAL, 24, 10, 0, 1, 3, 10,_none,_ethe, 1, 0, 0, 0},
    {SpeciesId_DOG          , 12,  4, 0, 2, 1,  2,_beas,_norm, 0, 0, 0, 0},
    {SpeciesId_ANT          , 12,  2, 0, 1, 0,  1,_beas,_norm, 0, 0, 0, 0},
    {SpeciesId_BEE          , 12,  2, 0, 3, 1,  2,_beas,_norm, 0, 0, 0, 1},
    {SpeciesId_BEETLE       , 24,  6, 0, 1, 0,  1,_beas,_norm, 0, 0, 0, 0},
    {SpeciesId_SCORPION     , 24,  5, 0, 1, 2,  3,_beas,_norm, 0, 0, 1, 0},
    {SpeciesId_SNAKE        , 24,  4, 0, 2, 1,  2,_beas,_norm, 0, 0, 0, 0},
    {SpeciesId_COBRA        , 24,  2, 0, 1, 2,  3,_beas,_norm, 0, 0, 0, 0},
};
#if __cpp_constexpr >= 201304
static bool constexpr _check_specieses() {
    for (int i = 0; i < SpeciesId_COUNT; i++)
        if (specieses[i].species_id != i)
            return false;
    return true;
}
static_assert(_check_specieses(), "missed a spot");
#endif

struct AbilityCooldown {
    AbilityId ability_id;
    int64_t expiration_time;
};

#endif
