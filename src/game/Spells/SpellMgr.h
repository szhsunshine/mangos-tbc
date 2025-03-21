/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SPELLMGR_H
#define _SPELLMGR_H

// For static or at-server-startup loaded spell data
// For more high level function for sSpellTemplate data

#include "Common.h"
#include "Globals/SharedDefines.h"
#include "Spells/SpellAuraDefines.h"
#include "Server/DBCStructure.h"
#include "Server/DBCStores.h"
#include "Entities/DynamicObject.h"
#include "Entities/GameObject.h"
#include "Entities/Corpse.h"
#include "Entities/Unit.h"
#include "Entities/Player.h"
#include "Spells/SpellAuras.h"
#include "Server/SQLStorages.h"

#include <map>

class Player;
class Spell;
class Unit;
struct SpellModifier;

// only used in code
enum SpellCategories
{
    SPELLCATEGORY_HEALTH_MANA_POTIONS = 4,
    SPELLCATEGORY_DEVOUR_MAGIC        = 12
};

// Spell clasification
enum SpellSpecific
{
    SPELL_NORMAL,
    SPELL_FOOD,
    SPELL_DRINK,
    SPELL_FOOD_AND_DRINK,
    SPELL_WELL_FED,
    SPELL_FLASK_ELIXIR,
    SPELL_SEAL,
    SPELL_JUDGEMENT,
    SPELL_BLESSING,
    SPELL_AURA,
    SPELL_STING,
    SPELL_ASPECT,
    SPELL_TRACKER,
    SPELL_CURSE,
    SPELL_SOUL_CAPTURE,
    SPELL_MAGE_ARMOR,
    SPELL_WARLOCK_ARMOR,
    SPELL_ELEMENTAL_SHIELD,
    // TBC+ specifics:
    SPELL_BATTLE_ELIXIR,
    SPELL_GUARDIAN_ELIXIR,
    SPELL_SHOUT_BUFF,
    SPELL_CORRUPTION,
};

SpellSpecific GetSpellSpecific(uint32 spellId);

// Different spell properties
inline float GetSpellRadius(SpellRadiusEntry const* radius) { return (radius ? radius->Radius : 0); }
uint32 GetSpellCastTime(SpellEntry const* spellInfo, Spell const* spell = nullptr);
uint32 GetSpellCastTimeForBonus(SpellEntry const* spellProto, DamageEffectType damagetype);
float CalculateDefaultCoefficient(SpellEntry const* spellProto, DamageEffectType const damagetype);
inline float GetSpellMinRange(SpellRangeEntry const* range) { return (range ? range->minRange : 0); }
inline float GetSpellMaxRange(SpellRangeEntry const* range) { return (range ? range->maxRange : 0); }
inline uint32 GetSpellRecoveryTime(SpellEntry const* spellInfo) { return spellInfo->RecoveryTime > spellInfo->CategoryRecoveryTime ? spellInfo->RecoveryTime : spellInfo->CategoryRecoveryTime; }
int32 GetSpellDuration(SpellEntry const* spellInfo);
int32 GetSpellMaxDuration(SpellEntry const* spellInfo);
int32 CalculateSpellDuration(SpellEntry const* spellInfo, Unit const* caster = nullptr);
uint16 GetSpellAuraMaxTicks(SpellEntry const* spellInfo);
uint16 GetSpellAuraMaxTicks(uint32 spellId);
WeaponAttackType GetWeaponAttackType(SpellEntry const* spellInfo);

inline bool IsSpellHaveEffect(SpellEntry const* spellInfo, SpellEffects effect)
{
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (SpellEffects(spellInfo->Effect[i]) == effect)
            return true;
    return false;
}

inline bool IsAuraApplyEffect(SpellEntry const* spellInfo, SpellEffectIndex effecIdx)
{
    switch (spellInfo->Effect[effecIdx])
    {
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_PERSISTENT_AREA_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            return true;
    }
    return false;
}

inline bool IsAuraApplyEffects(SpellEntry const* entry, SpellEffectIndexMask mask)
{
    if (!entry)
        return false;
    uint32 emptyMask = 0;
    for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
    {
        const uint32 current = (1 << i);
        if (entry->Effect[i])
        {
            if ((mask & current) && !IsAuraApplyEffect(entry, SpellEffectIndex(i)))
                return false;
        }
        else
            emptyMask |= current;
    }
    // Check if all queried effects are actually empty
    const bool empty = !(mask & ~emptyMask);
    return !empty;
}

inline bool IsSpellAppliesAura(SpellEntry const* spellInfo, uint32 effectMask = ((1 << EFFECT_INDEX_0) | (1 << EFFECT_INDEX_1) | (1 << EFFECT_INDEX_2)))
{
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (effectMask & (1 << i))
            if (IsAuraApplyEffect(spellInfo, SpellEffectIndex(i)))
                return true;

    return false;
}

inline bool IsEffectHandledOnDelayedSpellLaunch(SpellEntry const* spellInfo, SpellEffectIndex effecIdx)
{
    switch (spellInfo->Effect[effecIdx])
    {
        case SPELL_EFFECT_SCHOOL_DAMAGE:
        case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
        case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
        case SPELL_EFFECT_WEAPON_DAMAGE:
        case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            return true;
        default:
            return false;
    }
}

inline bool IsPeriodicRegenerateEffect(SpellEntry const* spellInfo, SpellEffectIndex effecIdx)
{
    switch (AuraType(spellInfo->EffectApplyAuraName[effecIdx]))
    {
        case SPELL_AURA_PERIODIC_ENERGIZE:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
            return true;
        default:
            return false;
    }
}

inline bool IsSpellHaveAura(SpellEntry const* spellInfo, AuraType aura, uint32 effectMask = (1 << EFFECT_INDEX_0) | (1 << EFFECT_INDEX_1) | (1 << EFFECT_INDEX_2))
{
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (effectMask & (1 << i))
            if (AuraType(spellInfo->EffectApplyAuraName[i]) == aura)
                return true;
    return false;
}

inline bool IsSpellLastAuraEffect(SpellEntry const* spellInfo, SpellEffectIndex effecIdx)
{
    for (int i = effecIdx + 1; i < MAX_EFFECT_INDEX; ++i)
        if (spellInfo->EffectApplyAuraName[i])
            return false;
    return true;
}

inline bool IsAllowingDeadTarget(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_EX2_CAN_TARGET_DEAD) || spellInfo->HasAttribute(SPELL_ATTR_PASSIVE) || spellInfo->Targets & (TARGET_FLAG_PVP_CORPSE | TARGET_FLAG_UNIT_CORPSE | TARGET_FLAG_CORPSE_ALLY);
}

inline bool IsSealSpell(SpellEntry const* spellInfo)
{
    // Collection of all the seal family flags. No other paladin spell has any of those.
    return spellInfo->IsFitToFamily(SPELLFAMILY_PALADIN, uint64(0x000004000A000200)) &&
           // avoid counting target triggered effect as seal for avoid remove it or seal by it.
           spellInfo->EffectImplicitTargetA[0] == TARGET_SELF;
}

inline bool IsSpellMagePolymorph(uint32 spellid)
{
    // Only mage polymorph bears hidden scripted regeneration
    const SpellEntry* entry = sSpellTemplate.LookupEntry<SpellEntry>(spellid);
    return (entry && entry->SpellFamilyName == SPELLFAMILY_MAGE && (entry->SpellFamilyFlags & uint64(0x1000000)) && IsSpellHaveAura(entry, SPELL_AURA_MOD_CONFUSE));
}

inline bool IsSpellEffectTriggerSpell(const SpellEntry* entry, SpellEffectIndex effIndex)
{
    if (!entry)
        return false;

    switch (entry->Effect[effIndex])
    {
        case SPELL_EFFECT_TRIGGER_MISSILE:
        case SPELL_EFFECT_TRIGGER_SPELL:
        case SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE:
        case SPELL_EFFECT_TRIGGER_SPELL_2:
            return true;
    }
    return false;
}

inline bool IsSpellEffectTriggerSpellByAura(const SpellEntry* entry, SpellEffectIndex effIndex)
{
    if (!entry || !IsAuraApplyEffect(entry, effIndex))
        return false;

    switch (entry->EffectApplyAuraName[effIndex])
    {
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        case SPELL_AURA_PROC_TRIGGER_SPELL:
        case SPELL_AURA_PROC_TRIGGER_DAMAGE:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
        case SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE:
            return true;
    }
    return false;
}

inline bool IsSpellEffectAbleToCrit(const SpellEntry* entry, SpellEffectIndex index)
{
    if (!entry || entry->HasAttribute(SPELL_ATTR_EX2_CANT_CRIT))
        return false;

    switch (entry->Effect[index])
    {
        case SPELL_EFFECT_SCHOOL_DAMAGE:
        case SPELL_EFFECT_HEAL:
        case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
        case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
        case SPELL_EFFECT_WEAPON_DAMAGE:
        case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            return true;
        case SPELL_EFFECT_ENERGIZE: // Mana Potion and similar spells, Lay on hands
            return (entry->EffectMiscValue[index] == POWER_MANA && entry->SpellFamilyName && entry->DmgClass);
    }
    return false;
}

inline bool IsSpellAbleToCrit(const SpellEntry* entry)
{
    if (!entry || entry->HasAttribute(SPELL_ATTR_EX2_CANT_CRIT))
        return false;

    for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (entry->Effect[i] && IsSpellEffectAbleToCrit(entry, SpellEffectIndex(i)))
            return true;
    }
    return false;
}

inline bool IsPassiveSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_PASSIVE);
}

inline bool IsPassiveSpell(uint32 spellId)
{
    const SpellEntry* entry = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
    return (entry && IsPassiveSpell(entry));
}

inline bool IsPassiveSpellStackableWithRanks(SpellEntry const* spellProto)
{
    if (!IsPassiveSpell(spellProto))
        return false;

    return !IsSpellHaveEffect(spellProto, SPELL_EFFECT_APPLY_AURA);
}

inline bool IsAutocastable(SpellEntry const* spellInfo)
{
    return !(spellInfo->HasAttribute(SPELL_ATTR_EX_UNAUTOCASTABLE_BY_CHARMED) || spellInfo->HasAttribute(SPELL_ATTR_PASSIVE));
}

inline bool IsAutocastable(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
    if (!spellInfo)
        return false;
    return IsAutocastable(spellInfo);
}

inline bool IsSpellRemoveAllMovementAndControlLossEffects(SpellEntry const* spellProto)
{
    return spellProto->EffectApplyAuraName[EFFECT_INDEX_0] == SPELL_AURA_MECHANIC_IMMUNITY &&
           spellProto->EffectMiscValue[EFFECT_INDEX_0] == 1 &&
           spellProto->EffectApplyAuraName[EFFECT_INDEX_1] == 0 &&
           spellProto->EffectApplyAuraName[EFFECT_INDEX_2] == 0 &&
           spellProto->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY);
}

inline bool IsDeathOnlySpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_EX3_CAST_ON_DEAD) || spellInfo->Id == 2584;
}

inline bool IsDeathPersistentSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_EX3_DEATH_PERSISTENT);
}

inline bool IsNonCombatSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_CANT_USED_IN_COMBAT);
}

// some creatures should run immediately after being summoned by spell
inline bool IsSpellSetRun(SpellEntry const* spellInfo)
{
    switch (spellInfo->Id)
    {
        case 39163:    // [DND]Rescue Wyvern
            return true;
        default:
            return false;
    }
}

bool IsExplicitPositiveTarget(uint32 targetA);
bool IsExplicitNegativeTarget(uint32 targetA);

inline bool IsBinarySpell(SpellEntry const* spellInfo)
{
    // Spell is considered binary if:
    // * Its composed entirely out of non-damage and aura effects (Example: Mind Flay, Vampiric Embrace, DoTs, etc)
    // * Has damage and non-damage effects with additional mechanics (Example: Death Coil, Frost Nova)
    uint32 effectmask = 0;  // A bitmask of effects: set bits are valid effects
    uint32 nondmgmask = 0;  // A bitmask of effects: set bits are non-damage effects
    uint32 mechmask = 0;    // A bitmask of effects: set bits are non-damage effects with additional mechanics
    for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!spellInfo->Effect[i] || IsSpellEffectTriggerSpell(spellInfo, SpellEffectIndex(i)))
            continue;

        effectmask |= (1 << i);

        bool damage = false;
        if (!spellInfo->EffectApplyAuraName[i])
        {
            // If its not an aura effect, check for damage effects
            switch (spellInfo->Effect[i])
            {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
                case SPELL_EFFECT_HEALTH_LEECH:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                //   SPELL_EFFECT_POWER_BURN: deals damage for power burned, but its either full damage or resist?
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    damage = true;
                    break;
            }
        }
        if (!damage)
        {
            nondmgmask |= (1 << i);
            if (spellInfo->EffectMechanic[i])
                mechmask |= (1 << i);
        }
    }
    // No non-damage involved at all: assumed all effects which should be rolled separately or no effects
    if (!effectmask || !nondmgmask)
        return false;
    // All effects are non-damage
    if (nondmgmask == effectmask)
        return true;
    // No non-damage effects with additional mechanics
    if (!mechmask)
        return false;
    // Binary spells execution order detection
    for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
    {
        // If effect is present and not a non-damage effect
        const uint32 effect = (1 << i);
        if ((effectmask & effect) && !(nondmgmask & effect))
        {
            // Iterate over mechanics
            for (uint32 e = EFFECT_INDEX_0; e < MAX_EFFECT_INDEX; ++e)
            {
                // If effect is extra mechanic on the same target as damage effect
                if ((mechmask & (1 << e)) &&
                    spellInfo->EffectImplicitTargetA[i] == spellInfo->EffectImplicitTargetA[e] &&
                    spellInfo->EffectImplicitTargetB[i] == spellInfo->EffectImplicitTargetB[e])
                {
                    return (e > i); // Post-2.3: checks the order of application
                }
            }
        }
    }
    return false;
}

inline bool IsCasterSourceTarget(uint32 target)
{
    switch (target)
    {
        case TARGET_SELF:
        case TARGET_PET:
        case TARGET_ALL_PARTY_AROUND_CASTER:
        case TARGET_IN_FRONT_OF_CASTER:
        case TARGET_MASTER:
        case TARGET_MINION:
        case TARGET_ALL_PARTY:
        case TARGET_ALL_PARTY_AROUND_CASTER_2:
        case TARGET_SELF_FISHING:
        case TARGET_TOTEM_EARTH:
        case TARGET_TOTEM_WATER:
        case TARGET_TOTEM_AIR:
        case TARGET_TOTEM_FIRE:
        case TARGET_AREAEFFECT_GO_AROUND_DEST:
        case TARGET_SELF2:
        case TARGET_NONCOMBAT_PET:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsSpellWithCasterSourceTargetsOnly(SpellEntry const* spellInfo)
{
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!spellInfo->Effect[i])                          // Skip junk in DBC
            continue;

        uint32 targetA = spellInfo->EffectImplicitTargetA[i];
        if (targetA && !IsCasterSourceTarget(targetA))
            return false;

        uint32 targetB = spellInfo->EffectImplicitTargetB[i];
        if (targetB && !IsCasterSourceTarget(targetB))
            return false;

        if (!targetA && !targetB)
            return false;
    }
    return true;
}

inline bool IsPointEffectTarget(Targets target)
{
    switch (target)
    {
        case TARGET_INNKEEPER_COORDINATES:
        case TARGET_TABLE_X_Y_Z_COORDINATES:
        case TARGET_CASTER_COORDINATES:
        case TARGET_SCRIPT_COORDINATES:
        case TARGET_CURRENT_ENEMY_COORDINATES:
        case TARGET_DUELVSPLAYER_COORDINATES:
        case TARGET_DYNAMIC_OBJECT_COORDINATES:
        case TARGET_POINT_AT_NORTH:
        case TARGET_POINT_AT_SOUTH:
        case TARGET_POINT_AT_EAST:
        case TARGET_POINT_AT_WEST:
        case TARGET_POINT_AT_NE:
        case TARGET_POINT_AT_NW:
        case TARGET_POINT_AT_SE:
        case TARGET_POINT_AT_SW:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsAreaEffectPossitiveTarget(Targets target)
{
    switch (target)
    {
        case TARGET_ALL_PARTY_AROUND_CASTER:
        case TARGET_ALL_FRIENDLY_UNITS_AROUND_CASTER:
        case TARGET_ALL_FRIENDLY_UNITS_IN_AREA:
        case TARGET_ALL_PARTY:
        case TARGET_ALL_PARTY_AROUND_CASTER_2:
        case TARGET_AREAEFFECT_PARTY:
        case TARGET_ALL_RAID_AROUND_CASTER:
        case TARGET_AREAEFFECT_PARTY_AND_CLASS:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsAreaEffectTarget(Targets target)
{
    switch (target)
    {
        case TARGET_AREAEFFECT_INSTANT:
        case TARGET_AREAEFFECT_CUSTOM:
        case TARGET_ALL_ENEMY_IN_AREA:
        case TARGET_ALL_ENEMY_IN_AREA_INSTANT:
        case TARGET_ALL_PARTY_AROUND_CASTER:
        case TARGET_IN_FRONT_OF_CASTER:
        case TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
        case TARGET_ALL_FRIENDLY_UNITS_AROUND_CASTER:
        case TARGET_ALL_FRIENDLY_UNITS_IN_AREA:
        case TARGET_ALL_PARTY:
        case TARGET_ALL_PARTY_AROUND_CASTER_2:
        case TARGET_AREAEFFECT_PARTY:
        case TARGET_AREAEFFECT_GO_AROUND_DEST:
        case TARGET_ALL_RAID_AROUND_CASTER:
        case TARGET_AREAEFFECT_PARTY_AND_CLASS:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsAreaOfEffectSpell(SpellEntry const* spellInfo)
{
    if (IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetA[EFFECT_INDEX_0])) || IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetB[EFFECT_INDEX_0])))
        return true;
    if (IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetA[EFFECT_INDEX_1])) || IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetB[EFFECT_INDEX_1])))
        return true;
    if (IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetA[EFFECT_INDEX_2])) || IsAreaEffectTarget(Targets(spellInfo->EffectImplicitTargetB[EFFECT_INDEX_2])))
        return true;
    return false;
}

inline bool IsAreaAuraEffect(uint32 effect)
{
    if (effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY    ||
            effect == SPELL_EFFECT_APPLY_AREA_AURA_FRIEND   ||
            effect == SPELL_EFFECT_APPLY_AREA_AURA_ENEMY    ||
            effect == SPELL_EFFECT_APPLY_AREA_AURA_PET      ||
            effect == SPELL_EFFECT_APPLY_AREA_AURA_OWNER)
        return true;
    return false;
}

inline bool HasAreaAuraEffect(SpellEntry const* spellInfo)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (IsAreaAuraEffect(spellInfo->Effect[i]))
            return true;
    return false;
}

inline bool IsPersistentAuraEffect(uint32 effect)
{
    if (effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
        return true;
    return false;
}

inline bool HasPersistentAuraEffect(SpellEntry const* spellInfo)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (IsPersistentAuraEffect(spellInfo->Effect[i]))
            return true;
    return false;
}

inline bool IsOnlySelfTargeting(SpellEntry const* spellInfo)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!spellInfo->Effect[i])
            return true;

        switch (spellInfo->EffectImplicitTargetA[i])
        {
            case TARGET_SELF:
            case TARGET_SELF2:
                break;
            default:
                return false;
        }
        switch (spellInfo->EffectImplicitTargetB[i])
        {
            case TARGET_SELF:
            case TARGET_SELF2:
            case TARGET_NONE:
                break;
            default:
                return false;
        }
    }
    return true;
}

inline bool IsSingleTargetSpell(SpellEntry const* spellInfo)
{
    // all other single target spells have if it has AttributesEx5
    if (spellInfo->HasAttribute(SPELL_ATTR_EX5_SINGLE_TARGET_SPELL))
        return true;

    // single target triggered spell.
    // Not real client side single target spell, but it' not triggered until prev. aura expired.
    // This is allow store it in single target spells list for caster for spell proc checking
    if (spellInfo->Id == 38324)                             // Regeneration (triggered by 38299 (HoTs on Heals))
        return true;

    return false;
}

inline bool IsSingleTargetSpells(SpellEntry const* spellInfo1, SpellEntry const* spellInfo2)
{
    return (IsSingleTargetSpell(spellInfo1) && (spellInfo1->Id == spellInfo2->Id || (spellInfo1->SpellFamilyFlags == spellInfo2->SpellFamilyFlags && IsSingleTargetSpell(spellInfo2))));
}

inline bool IsScriptTarget(uint32 target)
{
    switch (target)
    {
        case TARGET_SCRIPT:
        case TARGET_SCRIPT_COORDINATES:
        case TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT:
        case TARGET_AREAEFFECT_INSTANT:
        case TARGET_AREAEFFECT_CUSTOM:
        case TARGET_AREAEFFECT_GO_AROUND_SOURCE:
        case TARGET_AREAEFFECT_GO_AROUND_DEST:
        case TARGET_NARROW_FRONTAL_CONE:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsNeutralTarget(uint32 target)
{
    // This is an exhaustive list for demonstrativeness and global search reasons.
    // Also includes unknown targets, so we wont forget about them easily.
    // TODO: We need to research the unknown targets and list them under their proper category in the future.
    switch (target)
    {
        case TARGET_NONE:
        case TARGET_RANDOM_UNIT_CHAIN_IN_AREA:
        case TARGET_INNKEEPER_COORDINATES:
        case TARGET_11:
        case TARGET_TABLE_X_Y_Z_COORDINATES:
        case TARGET_EFFECT_SELECT:
        case TARGET_CASTER_COORDINATES:
        case TARGET_GAMEOBJECT:
        case TARGET_DUELVSPLAYER:
        case TARGET_GAMEOBJECT_ITEM:
        case TARGET_29:
        case TARGET_TOTEM_EARTH:
        case TARGET_TOTEM_WATER:
        case TARGET_TOTEM_AIR:
        case TARGET_TOTEM_FIRE:
        case TARGET_DYNAMIC_OBJECT_FRONT:
        case TARGET_DYNAMIC_OBJECT_BEHIND:
        case TARGET_DYNAMIC_OBJECT_LEFT_SIDE:
        case TARGET_DYNAMIC_OBJECT_RIGHT_SIDE:
        case TARGET_DEST_CASTER_FRONT_LEAP:
        case TARGET_58:
        case TARGET_DUELVSPLAYER_COORDINATES:
        case TARGET_INFRONT_OF_VICTIM:
        case TARGET_BEHIND_VICTIM:
        case TARGET_RIGHT_FROM_VICTIM:
        case TARGET_LEFT_FROM_VICTIM:
        case TARGET_70:
        case TARGET_RANDOM_NEARBY_LOC:
        case TARGET_RANDOM_CIRCUMFERENCE_POINT:
        case TARGET_74:
        case TARGET_RANDOM_CIRCUMFERENCE_AROUND_TARGET:
        case TARGET_DYNAMIC_OBJECT_COORDINATES:
        case TARGET_POINT_AT_NORTH:
        case TARGET_POINT_AT_SOUTH:
        case TARGET_POINT_AT_EAST:
        case TARGET_POINT_AT_WEST:
        case TARGET_POINT_AT_NE:
        case TARGET_POINT_AT_NW:
        case TARGET_POINT_AT_SE:
        case TARGET_POINT_AT_SW:
        case TARGET_RANDOM_NEARBY_DEST:
        case TARGET_88:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsFriendlyTarget(uint32 target)
{
    // This is an exhaustive list for demonstrativeness and global search reasons.
    switch (target)
    {
        case TARGET_SELF:
        case TARGET_RANDOM_FRIEND_CHAIN_IN_AREA:
        case TARGET_PET:
        case TARGET_ALL_PARTY_AROUND_CASTER:
        case TARGET_SINGLE_FRIEND:
        case TARGET_MASTER:
        case TARGET_ALL_FRIENDLY_UNITS_AROUND_CASTER:
        case TARGET_ALL_FRIENDLY_UNITS_IN_AREA:
        case TARGET_MINION:
        case TARGET_ALL_PARTY:
        case TARGET_ALL_PARTY_AROUND_CASTER_2:
        case TARGET_SINGLE_PARTY:
        case TARGET_AREAEFFECT_PARTY:
        case TARGET_SELF_FISHING:
        case TARGET_CHAIN_HEAL:
        case TARGET_ALL_RAID_AROUND_CASTER:
        case TARGET_SINGLE_FRIEND_2:
        case TARGET_FRIENDLY_FRONTAL_CONE:
        case TARGET_AREAEFFECT_PARTY_AND_CLASS:
        case TARGET_SELF2:
        case TARGET_NONCOMBAT_PET:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsHostileTarget(uint32 target)
{
    // This is an exhaustive list for demonstrativeness and global search reasons.
    switch (target)
    {
        case TARGET_RANDOM_ENEMY_CHAIN_IN_AREA:
        case TARGET_CHAIN_DAMAGE:
        case TARGET_ALL_ENEMY_IN_AREA:
        case TARGET_ALL_ENEMY_IN_AREA_INSTANT:
        case TARGET_IN_FRONT_OF_CASTER:
        case TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
        case TARGET_ALL_HOSTILE_UNITS_AROUND_CASTER:
        case TARGET_CURRENT_ENEMY_COORDINATES:
        case TARGET_LARGE_FRONTAL_CONE:
            return true;
        default:
            break;
    }
    return false;
}

inline bool IsEffectTargetScript(uint32 targetA, uint32 targetB)
{
    return (IsScriptTarget(targetA) || IsScriptTarget(targetB));
}

inline bool IsEffectTargetNeutral(uint32 targetA, uint32 targetB)
{
    return (IsNeutralTarget(targetA) && IsNeutralTarget(targetB));
}

inline bool IsEffectTargetPositive(uint32 targetA, uint32 targetB)
{
    return (IsFriendlyTarget(targetA) || IsFriendlyTarget(targetB));
}

inline bool IsEffectTargetNegative(uint32 targetA, uint32 targetB)
{
    return (IsHostileTarget(targetA) || IsHostileTarget(targetB));
}

inline bool IsNeutralEffectTargetPositive(uint32 etarget, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    switch (etarget)
    {
        case TARGET_RANDOM_UNIT_CHAIN_IN_AREA:
        case TARGET_11:
        case TARGET_DUELVSPLAYER:
        case TARGET_29:
        case TARGET_58:
        case TARGET_70:
        case TARGET_74:
        case TARGET_RANDOM_CIRCUMFERENCE_AROUND_TARGET:
            break;
        default:
            return true; // Some gameobjects or coords, who cares
    }
    if (!target || (target->GetTypeId() != TYPEID_PLAYER && target->GetTypeId() != TYPEID_UNIT))
        return true;

    if (caster == target)
        return true; // Early self-cast detection

    if (!caster)
        return true; // TODO: Nice to have additional in-depth research for default value for nullcaster

    const Unit* utarget = (const Unit*)target;
    switch (caster->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            return ((const Unit*)caster)->IsFriendlyTo(utarget);
        case TYPEID_GAMEOBJECT:
            return ((const GameObject*)caster)->IsFriendlyTo(utarget);
        case TYPEID_DYNAMICOBJECT:
            return ((const DynamicObject*)caster)->IsFriendlyTo(utarget);
        case TYPEID_CORPSE:
            return ((const Corpse*)caster)->IsFriendlyTo(utarget);
        default:
            break;
    }
    return true;
}

inline bool IsPositiveEffectTargetMode(const SpellEntry* entry, SpellEffectIndex effIndex, const WorldObject* caster = nullptr, const WorldObject* target = nullptr, bool recursive = false)
{
    if (!entry)
        return false;

    // Triggered spells case: prefer child spell via IsPositiveSpell()-like scan for triggered spell
    if (IsSpellEffectTriggerSpell(entry, effIndex))
    {
        const uint32 spellid = entry->EffectTriggerSpell[effIndex];
        // Its possible to go infinite cycle with triggered spells. We are interested to peek only at the first layer so far
        if (!recursive && spellid && (spellid != entry->Id))
        {
            if (const SpellEntry* triggered = sSpellTemplate.LookupEntry<SpellEntry>(spellid))
            {
                for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
                {
                    if (!IsPositiveEffectTargetMode(triggered, SpellEffectIndex(i), caster, target, true))
                        return false;
                }
            }
        }
        // For trigger effects target modes are inconsistent: we have invalid and coflicting ones
        // Let's try to ignore them completely
        return true;
    }

    const uint32 a = entry->EffectImplicitTargetA[effIndex];
    const uint32 b = entry->EffectImplicitTargetB[effIndex];

    if ((!a && !b) || IsEffectTargetPositive(a, b) || IsEffectTargetScript(a, b))
        return true;
    else if (IsEffectTargetNegative(a, b))
    {
        // Workaround: Passive talents with negative target modes are getting removed by ice block and similar effects
        // TODO: Fix removal of passives in appropriate places and remove the check below afterwards
        if (entry->HasAttribute(SPELL_ATTR_PASSIVE))
            return true;
        return false;
    }
    else if (IsEffectTargetNeutral(a, b))
        return (IsPointEffectTarget(Targets(b ? b : a)) || IsNeutralEffectTargetPositive((b ? b : a), caster, target));

    // If we ever get to this point, we have unhandled target. Gotta say something about it.
    if (entry->Effect[effIndex])
        DETAIL_LOG("IsPositiveEffectTargetMode: Spell %u's effect %u has unhandled targets (A:%u B:%u)", entry->Id, effIndex,
                   entry->EffectImplicitTargetA[effIndex], entry->EffectImplicitTargetB[effIndex]);
    return true;
}

inline bool IsPositiveEffect(const SpellEntry* spellproto, SpellEffectIndex effIndex, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    if (!spellproto)
        return false;

    switch (spellproto->Effect[effIndex])
    {
        case SPELL_EFFECT_SEND_TAXI:                // Some NPCs that send taxis are neutral, so target mode fails
        case SPELL_EFFECT_QUEST_COMPLETE:           // TODO: Spells with these effects should be casted by a proper caster to meet target mode.
        case SPELL_EFFECT_KILL_CREDIT_GROUP:        // Until then switch makes them all positive
        case SPELL_EFFECT_TELEPORT_UNITS:
        case SPELL_EFFECT_CREATE_ITEM:
        case SPELL_EFFECT_SUMMON_CHANGE_ITEM:
            return true;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:    // Always hostile effects
            return false;
            break;
        case SPELL_EFFECT_DUMMY:
            // some explicitly required dummy effect sets
            switch (spellproto->Id)
            {
                case 28441:                                 // AB Effect 000
                    return false;
                case 18153:                                 // Kodo Kombobulator
                    return true;
                default:
                    break;
            }
            break;
        case SPELL_EFFECT_SCRIPT_EFFECT:
            // some explicitly required script effect sets
            switch (spellproto->Id)
            {
                case 42436:                                 // Drink!
                    return true;
                default:
                    break;
            }
            break;
        // Aura exceptions:
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        {
            switch (spellproto->EffectApplyAuraName[effIndex])
            {
                case SPELL_AURA_DUMMY:
                {
                    // dummy aura can be positive or negative dependent from casted spell
                    switch (spellproto->Id)
                    {
                        case 13139:                         // net-o-matic special effect
                        case 44877:                         // Living Flare Master
                            return false;
                        case 39834:                         // Vimgol population tester
                        case 39851:                         // Vimgol population tester
                        case 39852:                         // Vimgol population tester
                        case 39853:                         // Vimgol population tester
                        case 39854:                         // Vimgol population tester
                        case 39921:                         // Vimgol Pentagram Beam
                            return true;
                        default:
                            break;
                    }
                    break;
                }
                case SPELL_AURA_MOD_MELEE_HASTE:
                {
                    switch (spellproto->Id)
                    {
                        case 38449:                         // Blessing of the Tides
                            return true;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    // Generic effect check: negative on negative targets, positive on positive targets
    return IsPositiveEffectTargetMode(spellproto, effIndex, caster, target);
}

inline bool IsPositiveAuraEffect(const SpellEntry* entry, SpellEffectIndex effIndex, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    return (IsAuraApplyEffect(entry, effIndex) && IsPositiveEffect(entry, effIndex, caster, target));
}

inline bool IsPositiveSpellTargetModeForSpecificTarget(const SpellEntry* entry, uint8 effectMask, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    if (!entry)
        return false;
    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (effectMask & (1 << i))
            if (entry->Effect[i] && !IsPositiveEffectTargetMode(entry, SpellEffectIndex(i), caster, target))
                return false;
    return true;
}

inline bool IsPositiveSpellTargetModeForSpecificTarget(uint32 spellId, uint8 effectMask, const WorldObject* caster, const WorldObject* target)
{
    if (!spellId)
        return false;
    return IsPositiveSpellTargetModeForSpecificTarget(sSpellTemplate.LookupEntry<SpellEntry>(spellId), effectMask, caster, target);
}

inline bool IsPositiveSpellTargetMode(const SpellEntry* entry, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    if (!entry)
        return false;
    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (entry->Effect[i] && !IsPositiveEffectTargetMode(entry, SpellEffectIndex(i), caster, target))
            return false;
    return true;
}

inline bool IsPositiveSpellTargetMode(uint32 spellId, const WorldObject* caster, const WorldObject* target)
{
    if (!spellId)
        return false;
    return IsPositiveSpellTargetMode(sSpellTemplate.LookupEntry<SpellEntry>(spellId), caster, target);
}

inline bool IsPositiveSpell(const SpellEntry* entry, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    if (!entry)
        return false;
    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (entry->Effect[i] && !IsPositiveEffect(entry, SpellEffectIndex(i), caster, target))
            return false;
    return true;
}

inline bool IsPositiveSpell(uint32 spellId, const WorldObject* caster = nullptr, const WorldObject* target = nullptr)
{
    if (!spellId)
        return false;
    return IsPositiveSpell(sSpellTemplate.LookupEntry<SpellEntry>(spellId), caster, target);
}

inline bool IsSpellDoNotReportFailure(SpellEntry const* spellInfo)
{
    switch (spellInfo->Id)
    {
        case 32172:     // Thrallmars/Honor holds favor trigger spell
            return true;
        default:
            return false;
    }
}

inline bool IsDispelSpell(SpellEntry const* spellInfo)
{
    return IsSpellHaveEffect(spellInfo, SPELL_EFFECT_DISPEL);
}

inline bool isSpellBreakStealth(SpellEntry const* spellInfo)
{
    return !spellInfo->HasAttribute(SPELL_ATTR_EX_NOT_BREAK_STEALTH);
}

inline bool IsAutoRepeatRangedSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_RANGED) && spellInfo->HasAttribute(SPELL_ATTR_EX2_AUTOREPEAT_FLAG);
}

inline bool IsSpellRequiresRangedAP(SpellEntry const* spellInfo)
{
    return (spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER && spellInfo->DmgClass != SPELL_DAMAGE_CLASS_MELEE);
}

inline uint32 GetAffectedTargets(SpellEntry const* spellInfo)
{
    // custom target amount cases
    switch (spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (spellInfo->Id)
            {
                case 802:                                   // Mutate Bug (AQ40, Emperor Vek'nilash)
                case 804:                                   // Explode Bug (AQ40, Emperor Vek'lor)
                case 23138:                                 // Gate of Shazzrah (MC, Shazzrah)
                case 28560:                                 // Summon Blizzard (Naxx, Sapphiron)
                case 30541:                                 // Blaze (Magtheridon)
                case 30572:                                 // Quake (Magtheridon)
                case 30769:                                 // Pick Red Riding Hood (Karazhan, Big Bad Wolf)
                case 30835:                                 // Infernal Relay (Karazhan, Prince Malchezaar)
                case 31347:                                 // Doom (Hyjal Summit, Azgalor)
                case 32312:                                 // Move 1 (Karazhan, Chess Event)
                case 33711:                                 // Murmur's Touch (Shadow Labyrinth, Murmur)
                case 37388:                                 // Move 2 (Karazhan, Chess Event)
                case 38794:                                 // Murmur's Touch (h) (Shadow Labyrinth, Murmur)
                case 39338:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Horde
                case 39342:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Alliance
                case 40834:                                 // Agonizing Flames (BT, Illidan Stormrage)
                case 41537:                                 // Summon Enslaved Soul (BT, Reliquary of Souls)
                case 44869:                                 // Spectral Blast (SWP, Kalecgos)
                case 45391:                                 // Summon Demonic Vapor (SWP, Felmyst)
                case 45785:                                 // Sinister Reflection Clone (SWP, Kil'jaeden)
                case 45892:                                 // Sinister Reflection (SWP, Kil'jaeden)
                case 45976:                                 // Open Portal (SWP, M'uru)
                case 46372:                                 // Ice Spear Target Picker (Slave Pens, Ahune)
                    return 1;
                case 10258:                                 // Awaken Vault Warder (Uldaman)
                case 28542:                                 // Life Drain (Naxx, Sapphiron)
                    return 2;
                case 30004:                                 // Flame Wreath (Karazhan, Shade of Aran)
                case 31298:                                 // Sleep (Hyjal Summit, Anetheron)
                case 39341:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Horde
                case 39344:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Alliance
                case 39992:                                 // Needle Spine Targeting (BT, Warlord Najentus)
                case 40869:                                 // Fatal Attraction (BT, Mother Shahraz)
                case 41303:                                 // Soul Drain (BT, Reliquary of Souls)
                case 41376:                                 // Spite (BT, Reliquary of Souls)
                    return 3;
                case 37676:                                 // Insidious Whisper (SSC, Leotheras the Blind)
                case 38028:                                 // Watery Grave (SSC, Morogrim Tidewalker)
                case 46650:                                 // Open Brutallus Back Door (SWP, Felmyst)
                    return 4;
                case 30843:                                 // Enfeeble (Karazhan, Prince Malchezaar)
                case 40243:                                 // Crushing Shadows (BT, Teron Gorefiend)
                case 42005:                                 // Bloodboil (BT, Gurtogg Bloodboil)
                case 45641:                                 // Fire Bloom (SWP, Kil'jaeden)
                    return 5;
                case 28796:                                 // Poison Bolt Volley (Naxx, Faerlina)
                case 29213:                                 // Curse of the Plaguebringer (Naxx, Noth the Plaguebringer)
                    return 10;
                case 25991:                                 // Poison Bolt Volley (AQ40, Pincess Huhuran)
                    return 15;
                case 46771:                                 // Flame Sear (SWP, Grand Warlock Alythess)
                    return urand(3, 5);
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            if (spellInfo->Id == 38194)                   // Blink
                return 1;
        }
        default:
            break;
    }

    return spellInfo->MaxAffectedTargets;
}

SpellCastResult GetErrorAtShapeshiftedCast(SpellEntry const* spellInfo, uint32 form);

inline bool IsChanneledSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_EX_CHANNELED_1) || spellInfo->HasAttribute(SPELL_ATTR_EX_CHANNELED_2);
}

inline bool IsNeedCastSpellAtFormApply(SpellEntry const* spellInfo, ShapeshiftForm form)
{
    if ((!spellInfo->HasAttribute(SPELL_ATTR_PASSIVE) && !spellInfo->HasAttribute(SPELL_ATTR_HIDDEN_CLIENTSIDE)) || !form)
        return false;

    // passive spells with SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT are already active without shapeshift, do no recast!
    return (spellInfo->Stances & (1 << (form - 1)) && !spellInfo->HasAttribute(SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT));
}

inline bool IsNeedCastSpellAtOutdoor(SpellEntry const* spellInfo)
{
    return (spellInfo->HasAttribute(SPELL_ATTR_OUTDOORS_ONLY) && spellInfo->HasAttribute(SPELL_ATTR_PASSIVE));
}

inline bool IsReflectableSpell(SpellEntry const* spellInfo)
{
    return spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC && !spellInfo->HasAttribute(SPELL_ATTR_ABILITY)
        && !spellInfo->HasAttribute(SPELL_ATTR_EX_CANT_BE_REFLECTED) && !spellInfo->HasAttribute(SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)
        && !spellInfo->HasAttribute(SPELL_ATTR_PASSIVE) && !IsPositiveSpell(spellInfo);
}

// Mostly required by spells that target a creature inside GO
inline bool IsIgnoreLosSpell(SpellEntry const* spellInfo)
{
    switch (spellInfo->Id)
    {
        case 36795:                                 // Cannon Channel
            return true;
        default:
            break;
    }

    return spellInfo->rangeIndex == 13 || spellInfo->HasAttribute(SPELL_ATTR_EX2_IGNORE_LOS);
}

inline bool NeedsComboPoints(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_EX_REQ_TARGET_COMBO_POINTS) || spellInfo->HasAttribute(SPELL_ATTR_EX_REQ_COMBO_POINTS);
}

inline SpellSchoolMask GetSpellSchoolMask(SpellEntry const* spellInfo)
{
    return SpellSchoolMask(spellInfo->SchoolMask);
}

inline uint32 GetSpellMechanicMask(SpellEntry const* spellInfo, uint32 effectMask)
{
    uint32 mask = 0;
    if (spellInfo->Mechanic)
        mask |= 1 << (spellInfo->Mechanic - 1);

    for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!(effectMask & (1 << i)))
            continue;

        if (spellInfo->EffectMechanic[i])
            mask |= 1 << (spellInfo->EffectMechanic[i] - 1);
    }

    return mask;
}

inline uint32 GetAllSpellMechanicMask(SpellEntry const* spellInfo)
{
    uint32 mask = 0;
    if (spellInfo->Mechanic)
        mask |= 1 << (spellInfo->Mechanic - 1);
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (spellInfo->EffectMechanic[i])
            mask |= 1 << (spellInfo->EffectMechanic[i] - 1);
    return mask;
}

inline Mechanics GetEffectMechanic(SpellEntry const* spellInfo, SpellEffectIndex effect)
{
    if (spellInfo->EffectMechanic[effect])
        return Mechanics(spellInfo->EffectMechanic[effect]);
    if (spellInfo->Mechanic)
        return Mechanics(spellInfo->Mechanic);
    return MECHANIC_NONE;
}

inline uint32 GetDispellMask(DispelType dispel)
{
    // If dispell all
    if (dispel == DISPEL_ALL)
        return DISPEL_ALL_MASK;
    else
        return (1 << dispel);
}

inline bool IsAuraAddedBySpell(uint32 auraType, uint32 spellId)
{
    SpellEntry const* spellproto = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
    if (!spellproto) return false;

    for (int i = 0; i < 3; i++)
        if (spellproto->EffectApplyAuraName[i] == auraType)
            return true;
    return false;
}

// Caster Centric Specific: A target cannot have more than one instance of specific per caster
// This a relaxed rule and does not automatically exclude multi-ranking, multi-ranking should be handled separately (usually on effect stacking level)
// Example: Curses. One curse per caster, Curse of Agony and Curse of Doom ranks are stackable among casters, the rest of curse stacking logic is handled on effect basis
inline bool IsSpellSpecificUniquePerCaster(SpellSpecific specific)
{
    switch (int32(specific))
    {
        case SPELL_BLESSING:
        case SPELL_AURA:
        case SPELL_STING:
        case SPELL_CURSE:
        case SPELL_ASPECT:
        case SPELL_SHOUT_BUFF:
        case SPELL_JUDGEMENT:
        case SPELL_SOUL_CAPTURE:
        case SPELL_CORRUPTION:
            return true;
        default:
            break;
    }
    return false;
}

// Target Centric Specific: A target cannot have more than one instance of specific applied to it
// This is a restrictive rule and automatically excludes multi-ranking possibility
// Example: Elemental Shield. No matter who it came from, only last one and the strongest one should stay.
inline bool IsSpellSpecificUniquePerTarget(SpellSpecific specific)
{
    switch (int32(specific))
    {
        case SPELL_SEAL:
        case SPELL_TRACKER:
        case SPELL_WARLOCK_ARMOR:
        case SPELL_MAGE_ARMOR:
        case SPELL_ELEMENTAL_SHIELD:
        case SPELL_WELL_FED:
        case SPELL_BATTLE_ELIXIR:
        case SPELL_GUARDIAN_ELIXIR:
        case SPELL_FLASK_ELIXIR:
        case SPELL_FOOD:
        case SPELL_DRINK:
        case SPELL_FOOD_AND_DRINK:
            return true;
        default:
            break;
    }
    return false;
}

// Compares two spell specifics
inline bool IsSpellSpecificIdentical(SpellSpecific specific, SpellSpecific specific2)
{
    if (specific == specific2)
        return true;
    // Compare combined specifics
    switch (int32(specific))
    {
        case SPELL_BATTLE_ELIXIR:
            return specific2 == SPELL_BATTLE_ELIXIR ||
                   specific2 == SPELL_FLASK_ELIXIR;
        case SPELL_GUARDIAN_ELIXIR:
            return specific2 == SPELL_GUARDIAN_ELIXIR ||
                   specific2 == SPELL_FLASK_ELIXIR;
        case SPELL_FLASK_ELIXIR:
            return specific2 == SPELL_BATTLE_ELIXIR ||
                   specific2 == SPELL_GUARDIAN_ELIXIR ||
                   specific2 == SPELL_FLASK_ELIXIR;
        case SPELL_FOOD:
            return specific2 == SPELL_FOOD ||
                   specific2 == SPELL_FOOD_AND_DRINK;
        case SPELL_DRINK:
            return specific2 == SPELL_DRINK ||
                   specific2 == SPELL_FOOD_AND_DRINK;
        case SPELL_FOOD_AND_DRINK:
            return specific2 == SPELL_FOOD ||
                   specific2 == SPELL_DRINK ||
                   specific2 == SPELL_FOOD_AND_DRINK;
        default:
            break;
    }
    return false;
}

inline bool IsSimilarAuraEffect(SpellEntry const* entry, uint32 effect, SpellEntry const* entry2, uint32 effect2)
{
    return (entry2->EffectApplyAuraName[effect2] && entry->EffectApplyAuraName[effect] &&
            entry2->Effect[effect2] == entry->Effect[effect] &&
            entry2->EffectApplyAuraName[effect2] == entry->EffectApplyAuraName[effect] &&
            IsPositiveEffect(entry2, SpellEffectIndex(effect2)) == IsPositiveEffect(entry, SpellEffectIndex(effect)));
}

inline bool IsStackableAuraEffect(SpellEntry const* entry, SpellEntry const* entry2, uint32 i, Unit* pTarget = nullptr)
{
    const uint32 aura = entry->EffectApplyAuraName[i];
    // Ignore non-aura effects
    if (!aura)
        return true;

    // Get first similar - second spell's same aura with the same sign
    uint32 similar = EFFECT_INDEX_0;
    for (uint32 e = EFFECT_INDEX_0; e < MAX_EFFECT_INDEX; ++e)
    {
        if (IsSimilarAuraEffect(entry, i, entry2, e))
        {
            similar = e;
            break;
        }
        else if (e == (MAX_EFFECT_INDEX - 1))
            return true; // No similarities
    }

    // Special rule for food buffs
    if (GetSpellSpecific(entry->Id) == SPELL_WELL_FED && GetSpellSpecific(entry2->Id) != SPELL_WELL_FED)
        return true;

    // Short alias
    const bool positive = (IsPositiveEffect(entry, SpellEffectIndex(i)));
    const bool related = (entry->SpellFamilyName == entry2->SpellFamilyName);
    const bool siblings = (entry->SpellFamilyFlags == entry2->SpellFamilyFlags);
    const bool player = (entry->SpellFamilyName && !entry->SpellFamilyFlags.Empty());
    const bool multirank = (related && siblings && player);
    const bool instance = (entry->Id == entry2->Id || multirank);
    const bool icon = (entry->SpellIconID == entry2->SpellIconID); // Old bad practice, but a few old spells detection may still depend on it
    const bool visual = (entry->SpellVisual == entry2->SpellVisual); // Old bad practice, but a few old spells detection may still depend on it

    // If aura makes spell not multi-instanceable (do not stack the same spell id or ranks of this spell)
    bool nonmui = false;

    // Stack rules
    switch (aura)
    {
        // Dummy/Triggers
        case SPELL_AURA_DUMMY:
        case SPELL_AURA_PERIODIC_DUMMY:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        {
            if (!related)
                break;
            switch (entry->SpellFamilyName)
            {
                case SPELLFAMILY_GENERIC:
                    if (entry->SpellIconID == 92 && entry->SpellVisual == 99 && icon && visual)
                        return false; // Soulstone Resurrection
                    break;
                case SPELLFAMILY_SHAMAN:
                    if (entry->IsFitToFamilyMask(uint64(0x200)) && multirank)
                        return true; // Shaman Reincarnation (Passive) and Twisting Nether
                    break;
                case SPELLFAMILY_DRUID:
                    if (entry->IsFitToFamilyMask(uint64(0x44000000000)) && entry2->IsFitToFamilyMask(uint64(0x44000000000)))
                        return false; // Mangle (Cat) & Mangle (Bear)
                    if (entry->IsFitToFamilyMask(uint64(0x80)) && multirank)
                        return true; // Tranquility
                    if (entry->IsFitToFamilyMask(uint64(0x01000000000)))
                        return true; // Lifebloom
                    break;
            }
            break;
        }
        // DoT
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
            if (pTarget && pTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
                return false;
            break;
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_POWER_BURN_MANA:
            return true;
            break;
        // HoT
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_OBS_MOD_MANA:
            return true;
            break;
        // Raid debuffs: Hunter's Mark and Expose Weakness stack with each other, but not itself
        case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS:
        case SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS:
            return (!related || !siblings);
            break;
        case SPELL_AURA_MOD_ATTACK_POWER:
            // Attack Power debuffs logic: Do not stack Curse of Weakness, Demoralizing Roars/Shouts
            if (!positive && entry->EffectBasePoints[i] < 1 && entry2->EffectBasePoints[similar] < 1)
                return (!entry->SpellFamilyName && !entry->SpellFamilyName);
            break;
        // Armor & Resistance buffs and debuffs logic
        case SPELL_AURA_MOD_RESISTANCE:
        {
            if (entry->EffectMiscValue[i] != entry2->EffectMiscValue[similar])
                break;
            if (positive)
            {
                // Used as a non-zero type in this context
                const bool type = (entry->Dispel && entry->Dispel == entry2->Dispel);
                const bool family = (entry->SpellFamilyName || entry2->SpellFamilyName);
                const bool uncategorized = (!entry->SpellFamilyName || !entry2->SpellFamilyName);
                if (type && family && uncategorized)
                    return false; // Do not stack player buffs with scrolls
                const bool attacktable = (entry->DmgClass && entry->DmgClass == entry2->DmgClass);
                if (attacktable && !entry->SpellFamilyName && !entry2->SpellFamilyName)
                    return false; // Do not stack scrolls with other some procs (such Hyjal ring)
            }
            else
            {
                const bool type = (entry->Dispel == entry2->Dispel);
                const bool prevention = (entry->PreventionType && entry->PreventionType == entry2->PreventionType);
                if (type && prevention)
                    return false;
            }
            return true;
        }
        case SPELL_AURA_MOD_RESISTANCE_PCT:
        {
            if (entry->EffectMiscValue[i] != entry2->EffectMiscValue[similar])
                break;
            if (positive && entry->Dispel && entry->Dispel == entry2->Dispel)
                return false; // Inspiration / Ancestral Fortitude
            nonmui = true;
            break;
        }
        // By default base stats cannot stack if they're similar
        case SPELL_AURA_MOD_STAT:
        {
            if (entry->EffectMiscValue[i] != entry2->EffectMiscValue[similar])
                break;
            if (positive)
            {
                // Used as a non-zero type in this context
                const bool type = (entry->Dispel && entry->Dispel == entry2->Dispel);
                const bool family = (entry->SpellFamilyName || entry2->SpellFamilyName);
                const bool uncategorized = (!entry->SpellFamilyName || !entry2->SpellFamilyName);
                if (type && family && uncategorized)
                    return false; // Do not stack player buffs with scrolls
            }
            else
            {
                // Debuffs are based on patch notes decription of Shrink effects stacking
                // Used as a non-zero type in this context
                const bool type = (entry->Dispel && entry->Dispel == entry2->Dispel);
                if (type)
                    return false;
                nonmui = true;
            }
            break;
        }
        case SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE:
        case SPELL_AURA_MOD_PERCENT_STAT:
            nonmui = true;
            break;
        case SPELL_AURA_MOD_HEALING_DONE:
        case SPELL_AURA_MOD_HEALING_PCT:
            // Do not stack similar debuffs: Mortal Strike, Aimed Shot, Hex of Weakness
            if (!positive)
                return (entry->EffectMiscValue[i] == entry2->EffectMiscValue[similar]);
            break;
        case SPELL_AURA_MOD_MELEE_HASTE:
        case SPELL_AURA_MOD_RANGED_HASTE:
        case SPELL_AURA_MOD_DAMAGE_DONE:
        case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE: // Ferocious Inspiration, Shadow Embrace
            if (positive)
                return true;
            nonmui = true;
            break;
        case SPELL_AURA_MOD_DAMAGE_TAKEN:
        case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
        case SPELL_AURA_MOD_DECREASE_SPEED: // Bonus stacking handled by core
        case SPELL_AURA_MOD_INCREASE_SPEED: // Bonus stacking handled by core
        case SPELL_AURA_MOD_HEALTH_REGEN_PERCENT:
        case SPELL_AURA_PREVENTS_FLEEING:
            nonmui = true;
            break;
        case SPELL_AURA_PROC_TRIGGER_SPELL:
            if (instance && !icon)
                // Exception: Judgement of Light and Judgement of Wisdom have exact same spell family flags
                // Comparing icons is the fastest (but hacky) way to destinguish between two without poking spell chain
                break;
            nonmui = true;
            break;
        case SPELL_AURA_MOD_FEAR: // Fear/confuse effects: do not stack with the same mechanic type
        case SPELL_AURA_MOD_CONFUSE:
            return (entry->Mechanic != entry2->Mechanic);
            break;
        case SPELL_AURA_MOD_STUN: // Stun/root effects: prefer refreshing (overwrite) existing types if possible
        case SPELL_AURA_MOD_ROOT:
            if (entry->Mechanic != entry2->Mechanic)
                return true;
            nonmui = true;
            break;
        case SPELL_AURA_MOD_RATING: // Whitelisted, Rejuvenation has this
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE: // Party auras whitelist for Totem of Wrath
        case SPELL_AURA_MOD_SPELL_HIT_CHANCE: // Party auras whitelist for Totem of Wrath
        case SPELL_AURA_SPELL_MAGNET: // Party auras whitelist for Grounding Totem
            return true; // Always stacking auras
            break;
        case SPELL_AURA_MOD_POSSESS: // Mind control derrivatives
        case SPELL_AURA_MOD_POSSESS_PET: // Eyes of the beast
        case SPELL_AURA_MOD_CHARM: // Temporary Enslave/Tame derrivatives
        case SPELL_AURA_AOE_CHARM: // Mass charm by Boss/NPC
        case SPELL_AURA_DAMAGE_SHIELD: // Damage shields: Fire Shield, Thorns...
        case SPELL_AURA_MOD_SHAPESHIFT: // Forms and stances
        case SPELL_AURA_MOUNTED: // Mount
        case SPELL_AURA_EMPATHY: // Beast Lore
        // TODO: Make these exclusive rather than unstackable physically and move to nonmui:
        case SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK: // Heroism, Bloodlust, Icy Veins, Power Infusion
        case SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK: // Mounted Speed effects
        case SPELL_AURA_HASTE_SPELLS: // Post 2.0 Mind-numbing Poison and Curse of Tongues
            return false; // Never stacking auras
            break;
    }
    if (nonmui && instance && !IsChanneledSpell(entry) && !IsChanneledSpell(entry2))
        return false; // Forbids multi-ranking and multi-application on rule, exclude channeled spells (like Mind Flay)

    if (multirank && IsPositiveSpell(entry) && IsPositiveSpell(entry2))
        return false; // Forbids multi-ranking for positive spells

    return true;
}

inline bool IsStackableSpell(SpellEntry const* entry, SpellEntry const* entry2, Unit* pTarget = nullptr)
{
    for (uint32 i = EFFECT_INDEX_0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (entry->Effect[i] && entry->EffectApplyAuraName[i] && !IsStackableAuraEffect(entry, entry2, i, pTarget))
            return false;
    }
    return true;
}

inline bool IsSimilarExistingAuraStronger(const SpellAuraHolder* holder, const SpellAuraHolder* existing)
{
    if (!holder || !existing)
        return false;
    const SpellEntry* entry = holder->GetSpellProto();
    const SpellEntry* entry2 = existing->GetSpellProto();
    if (!entry || !entry2)
        return false;

    for (uint32 e = EFFECT_INDEX_0; e < MAX_EFFECT_INDEX; ++e)
    {
        for (uint32 e2 = EFFECT_INDEX_0; e2 < MAX_EFFECT_INDEX; ++e2)
        {
            if (IsSimilarAuraEffect(entry, e, entry2, e2))
            {
                Aura* aura1 = holder->GetAuraByEffectIndex(SpellEffectIndex(e));
                Aura* aura2 = existing->GetAuraByEffectIndex(SpellEffectIndex(e2));
                int32 value = aura1 ? (aura1->GetModifier()->m_amount / int32(aura1->GetStackAmount())) : 0;
                int32 value2 = aura2 ? (aura2->GetModifier()->m_amount / int32(aura2->GetStackAmount())) : 0;
                if (value < 0 && value2 < 0)
                {
                    value = abs(value);
                    value2 = abs(value2);
                }
                if (value2 > value)
                    return true;
            }
        }
    }
    return false;
}

inline bool IsSimilarExistingAuraStronger(const Unit* caster, const SpellEntry* entry, const SpellAuraHolder* existing)
{
    if (!caster || !existing)
        return false;
    const SpellEntry* entry2 = existing->GetSpellProto();
    if (!entry || !entry2)
        return false;

    for (uint32 e = EFFECT_INDEX_0; e < MAX_EFFECT_INDEX; ++e)
    {
        for (uint32 e2 = EFFECT_INDEX_0; e2 < MAX_EFFECT_INDEX; ++e2)
        {
            if (IsSimilarAuraEffect(entry, e, entry2, e2))
            {
                Aura* aura = existing->GetAuraByEffectIndex(SpellEffectIndex(e2));
                int32 value = entry->CalculateSimpleValue(SpellEffectIndex(e));
                int32 value2 = aura ? (aura->GetModifier()->m_amount / int32(aura->GetStackAmount())) : 0;
                // FIXME: We need API to peacefully pre-calculate static base spell damage without destroying mods
                // Until then this is a rather lame set of hacks
                // Apply combo points base damage for spells like expose armor
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    const Player* player = (const Player*)caster;
                    const Unit* target = existing->GetTarget();
                    const float comboDamage = entry->EffectPointsPerComboPoint[e];
                    if (player && target && (target->GetObjectGuid() == player->GetComboTargetGuid()))
                        value += int32(comboDamage * player->GetComboPoints());
                }
                if (value < 0 && value2 < 0)
                {
                    value = abs(value);
                    value2 = abs(value2);
                }
                if (value2 > value)
                    return true;
            }
        }
    }
    return false;
}

inline bool IsSimilarExistingAuraStronger(const Unit* caster, uint32 spellid, const SpellAuraHolder* existing)
{
    if (!spellid)
        return false;
    return IsSimilarExistingAuraStronger(caster, sSpellTemplate.LookupEntry<SpellEntry>(spellid), existing);
}

// Diminishing Returns interaction with spells
DiminishingGroup GetDiminishingReturnsGroupForSpell(SpellEntry const* spellproto, bool triggered);
bool IsDiminishingReturnsGroupDurationLimited(DiminishingGroup group);
DiminishingReturnsType GetDiminishingReturnsGroupType(DiminishingGroup group);

// Spell affects related declarations (accessed using SpellMgr functions)
typedef std::map<uint32, uint64> SpellAffectMap;

// Spell proc event related declarations (accessed using SpellMgr functions)
enum ProcFlags
{
    PROC_FLAG_NONE                          = 0x00000000,

    PROC_FLAG_KILLED                        = 0x00000001,   // 00 Killed by aggressor
    PROC_FLAG_KILL                          = 0x00000002,   // 01 Kill target (in most cases need XP/Honor reward, see Unit::IsTriggeredAtSpellProcEvent for additinoal check)

    PROC_FLAG_SUCCESSFUL_MELEE_HIT          = 0x00000004,   // 02 Successful melee auto attack
    PROC_FLAG_TAKEN_MELEE_HIT               = 0x00000008,   // 03 Taken damage from melee auto attack hit

    PROC_FLAG_SUCCESSFUL_MELEE_SPELL_HIT    = 0x00000010,   // 04 Successful attack by Spell that use melee weapon
    PROC_FLAG_TAKEN_MELEE_SPELL_HIT         = 0x00000020,   // 05 Taken damage by Spell that use melee weapon

    PROC_FLAG_SUCCESSFUL_RANGED_HIT         = 0x00000040,   // 06 Successful Ranged auto attack
    PROC_FLAG_TAKEN_RANGED_HIT              = 0x00000080,   // 07 Taken damage from ranged auto attack

    PROC_FLAG_SUCCESSFUL_RANGED_SPELL_HIT   = 0x00000100,   // 08 Successful Ranged attack by Spell that use ranged weapon
    PROC_FLAG_TAKEN_RANGED_SPELL_HIT        = 0x00000200,   // 09 Taken damage by Spell that use ranged weapon

    PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS     = 0x00000400,  // 10 Done positive spell that has dmg class none
    PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS    = 0x00000800,  // 11 Taken positive spell that has dmg class none

    PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG     = 0x00001000,  // 12 Done negative spell that has dmg class none
    PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG    = 0x00002000,  // 13 Taken negative spell that has dmg class none

    PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS    = 0x00004000,  // 14 Successful cast positive spell (by default only on healing)
    PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS   = 0x00008000,  // 15 Taken positive spell hit (by default only on healing)

    PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG    = 0x00010000,  // 16 Successful negative spell cast (by default only on damage)
    PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG   = 0x00020000,  // 17 Taken negative spell (by default only on damage)

    PROC_FLAG_ON_DO_PERIODIC                = 0x00040000,   // 18 Successful do periodic (damage / healing, determined by PROC_EX_PERIODIC_POSITIVE or negative if no procEx)
    PROC_FLAG_ON_TAKE_PERIODIC              = 0x00080000,   // 19 Taken spell periodic (damage / healing, determined by PROC_EX_PERIODIC_POSITIVE or negative if no procEx)

    PROC_FLAG_TAKEN_ANY_DAMAGE              = 0x00100000,   // 20 Taken any damage
    PROC_FLAG_ON_TRAP_ACTIVATION            = 0x00200000,   // 21 On trap activation

    PROC_FLAG_TAKEN_OFFHAND_HIT             = 0x00400000,   // 22 Taken off-hand melee attacks(not used)
    PROC_FLAG_SUCCESSFUL_OFFHAND_HIT        = 0x00800000    // 23 Successful off-hand melee attacks
};

#define MELEE_BASED_TRIGGER_MASK (PROC_FLAG_SUCCESSFUL_MELEE_HIT        | \
                                  PROC_FLAG_TAKEN_MELEE_HIT             | \
                                  PROC_FLAG_SUCCESSFUL_MELEE_SPELL_HIT  | \
                                  PROC_FLAG_TAKEN_MELEE_SPELL_HIT       | \
                                  PROC_FLAG_SUCCESSFUL_RANGED_HIT       | \
                                  PROC_FLAG_TAKEN_RANGED_HIT            | \
                                  PROC_FLAG_SUCCESSFUL_RANGED_SPELL_HIT | \
                                  PROC_FLAG_TAKEN_RANGED_SPELL_HIT)

#define NEGATIVE_TRIGGER_MASK (MELEE_BASED_TRIGGER_MASK                | \
                               PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG      | \
                               PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG           | \
                               PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG | \
                               PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG)

enum ProcFlagsEx
{
    PROC_EX_NONE                = 0x0000000,                // If none can tigger on Hit/Crit only (passive spells MUST defined by SpellFamily flag)
    PROC_EX_NORMAL_HIT          = 0x0000001,                // If set only from normal hit (only damage spells)
    PROC_EX_CRITICAL_HIT        = 0x0000002,
    PROC_EX_MISS                = 0x0000004,
    PROC_EX_RESIST              = 0x0000008,
    PROC_EX_DODGE               = 0x0000010,
    PROC_EX_PARRY               = 0x0000020,
    PROC_EX_BLOCK               = 0x0000040,
    PROC_EX_EVADE               = 0x0000080,
    PROC_EX_IMMUNE              = 0x0000100,
    PROC_EX_DEFLECT             = 0x0000200,
    PROC_EX_ABSORB              = 0x0000400,
    PROC_EX_REFLECT             = 0x0000800,
    PROC_EX_INTERRUPT           = 0x0001000,                // Melee hit result can be Interrupt (not used)
    PROC_EX_RESERVED1           = 0x0002000,
    PROC_EX_RESERVED2           = 0x0004000,
    PROC_EX_RESERVED3           = 0x0008000,
    PROC_EX_EX_TRIGGER_ALWAYS   = 0x0010000,                // If set trigger always ( no matter another flags) used for drop charges
    PROC_EX_EX_ONE_TIME_TRIGGER = 0x0020000,                // If set trigger always but only one time (not used)
    PROC_EX_PERIODIC_POSITIVE   = 0x0040000,                 // For periodic heal

    // Flags for internal use - do not use these in db!
    PROC_EX_INTERNAL_HOT        = 0x2000000
};

struct SpellProcEventEntry
{
    uint32      schoolMask;
    uint32      spellFamilyName;                            // if nonzero - for matching proc condition based on candidate spell's SpellFamilyNamer value
    ClassFamilyMask spellFamilyMask[MAX_EFFECT_INDEX];      // if nonzero - for matching proc condition based on candidate spell's SpellFamilyFlags  (like auras 107 and 108 do)
    uint32      procFlags;                                  // bitmask for matching proc event
    uint32      procEx;                                     // proc Extend info (see ProcFlagsEx)
    float       ppmRate;                                    // for melee (ranged?) damage spells - proc rate per minute. if zero, falls back to flat chance from Spell.dbc
    float       customChance;                               // Owerride chance (in most cases for debug only)
    uint32      cooldown;                                   // hidden cooldown used for some spell proc events, applied to _triggered_spell_
};

struct SpellBonusEntry
{
    float  direct_damage;
    float  dot_damage;
    float  ap_bonus;
    float  ap_dot_bonus;
};

typedef std::unordered_map<uint32, SpellProcEventEntry> SpellProcEventMap;
typedef std::unordered_map<uint32, SpellBonusEntry>     SpellBonusMap;

#define ELIXIR_BATTLE_MASK    0x01
#define ELIXIR_GUARDIAN_MASK  0x02
#define ELIXIR_FLASK_MASK     (ELIXIR_BATTLE_MASK|ELIXIR_GUARDIAN_MASK)
#define ELIXIR_UNSTABLE_MASK  0x04
#define ELIXIR_SHATTRATH_MASK 0x08
#define ELIXIR_WELL_FED       0x10                          // Some foods have SPELLFAMILY_POTION

struct SpellThreatEntry
{
    uint16 threat;
    float multiplier;
    float ap_bonus;
};

typedef std::map<uint32, uint8> SpellElixirMap;
typedef std::map<uint32, float> SpellProcItemEnchantMap;
typedef std::map<uint32, SpellThreatEntry> SpellThreatMap;

// Spell script target related declarations (accessed using SpellMgr functions)
enum SpellTargetType
{
    SPELL_TARGET_TYPE_GAMEOBJECT = 0,
    SPELL_TARGET_TYPE_CREATURE   = 1,
    SPELL_TARGET_TYPE_DEAD       = 2
};

#define MAX_SPELL_TARGET_TYPE 3

// pre-defined targeting for spells
struct SpellTargetEntry
{
    uint32 spellId;
    uint32 type;
    uint32 targetEntry;
    uint32 inverseEffectMask;

    bool CanNotHitWithSpellEffect(SpellEffectIndex effect) const { return !!(inverseEffectMask & (1 << effect)); }
};

// coordinates for spells (accessed using SpellMgr functions)
struct SpellTargetPosition
{
    uint32 target_mapId;
    float  target_X;
    float  target_Y;
    float  target_Z;
    float  target_Orientation;
};

typedef std::unordered_map<uint32, SpellTargetPosition> SpellTargetPositionMap;

// Spell pet auras
class PetAura
{
    public:
        PetAura() :
            removeOnChangePet(false),
            damage(0)
        {
        }

        PetAura(uint32 petEntry, uint32 aura, bool _removeOnChangePet, int _damage) :
            removeOnChangePet(_removeOnChangePet), damage(_damage)
        {
            auras[petEntry] = aura;
        }

        uint32 GetAura(uint32 petEntry) const
        {
            std::map<uint32, uint32>::const_iterator itr = auras.find(petEntry);
            if (itr != auras.end())
                return itr->second;
            else
            {
                std::map<uint32, uint32>::const_iterator itr2 = auras.find(0);
                if (itr2 != auras.end())
                    return itr2->second;
                else
                    return 0;
            }
        }

        void AddAura(uint32 petEntry, uint32 aura)
        {
            auras[petEntry] = aura;
        }

        bool IsRemovedOnChangePet() const
        {
            return removeOnChangePet;
        }

        int32 GetDamage() const
        {
            return damage;
        }

    private:
        std::map<uint32, uint32> auras;
        bool removeOnChangePet;
        int32 damage;
};
typedef std::map<uint16, PetAura> SpellPetAuraMap;

struct SpellArea
{
    uint32 spellId;
    uint32 areaId;                                          // zone/subzone/or 0 is not limited to zone
    uint32 questStart;                                      // quest start (quest must be active or rewarded for spell apply)
    uint32 questEnd;                                        // quest end (quest don't must be rewarded for spell apply)
    uint16 conditionId;                                     // conditionId - will replace questStart, questEnd, raceMask, gender and questStartCanActive
    int32  auraSpell;                                       // spell aura must be applied for spell apply )if possitive) and it don't must be applied in other case
    uint32 raceMask;                                        // can be applied only to races
    Gender gender;                                          // can be applied only to gender
    bool questStartCanActive;                               // if true then quest start can be active (not only rewarded)
    bool autocast;                                          // if true then auto applied at area enter, in other case just allowed to cast

    // helpers
    bool IsFitToRequirements(Player const* player, uint32 newZone, uint32 newArea) const;
    void ApplyOrRemoveSpellIfCan(Player* player, uint32 newZone, uint32 newArea, bool onlyApply) const;
};

typedef std::multimap<uint32 /*applySpellId*/, SpellArea> SpellAreaMap;
typedef std::multimap<uint32 /*auraSpellId*/, SpellArea const*> SpellAreaForAuraMap;
typedef std::multimap<uint32 /*areaOrZoneId*/, SpellArea const*> SpellAreaForAreaMap;
typedef std::pair<SpellAreaMap::const_iterator, SpellAreaMap::const_iterator> SpellAreaMapBounds;
typedef std::pair<SpellAreaForAuraMap::const_iterator, SpellAreaForAuraMap::const_iterator>  SpellAreaForAuraMapBounds;
typedef std::pair<SpellAreaForAreaMap::const_iterator, SpellAreaForAreaMap::const_iterator>  SpellAreaForAreaMapBounds;


// Spell rank chain  (accessed using SpellMgr functions)
struct SpellChainNode
{
    uint32 prev;
    uint32 first;
    uint32 req;
    uint8  rank;
};

typedef std::unordered_map<uint32, SpellChainNode> SpellChainMap;
typedef std::multimap<uint32, uint32> SpellChainMapNext;

// Spell learning properties (accessed using SpellMgr functions)
struct SpellLearnSkillNode
{
    uint16 skill;
    uint16 step;
    uint16 value;                                           // 0  - max skill value for player level
    uint16 maxvalue;                                        // 0  - max skill value for player level
};

typedef std::map<uint32, SpellLearnSkillNode> SpellLearnSkillMap;

struct SpellLearnSpellNode
{
    uint32 spell;
    bool active;                                            // show in spellbook or not
    bool autoLearned;
};

typedef std::multimap<uint32, SpellLearnSpellNode> SpellLearnSpellMap;
typedef std::pair<SpellLearnSpellMap::const_iterator, SpellLearnSpellMap::const_iterator> SpellLearnSpellMapBounds;

typedef std::multimap<uint32, SkillLineAbilityEntry const*> SkillLineAbilityMap;
typedef std::pair<SkillLineAbilityMap::const_iterator, SkillLineAbilityMap::const_iterator> SkillLineAbilityMapBounds;

typedef std::multimap<uint32, SkillRaceClassInfoEntry const*> SkillRaceClassInfoMap;
typedef std::pair<SkillRaceClassInfoMap::const_iterator, SkillRaceClassInfoMap::const_iterator> SkillRaceClassInfoMapBounds;

bool IsPrimaryProfessionSkill(uint32 skill);

inline bool IsProfessionSkill(uint32 skill)
{
    return  IsPrimaryProfessionSkill(skill) || skill == SKILL_FISHING || skill == SKILL_COOKING || skill == SKILL_FIRST_AID;
}

inline bool IsProfessionOrRidingSkill(uint32 skill)
{
    return  IsProfessionSkill(skill) || skill == SKILL_RIDING;
}

class SpellMgr
{
        friend struct DoSpellBonuses;
        friend struct DoSpellProcEvent;
        friend struct DoSpellProcItemEnchant;

        // Constructors
    public:
        SpellMgr();
        ~SpellMgr();

        // Accessors (const or static functions)
    public:
        // Spell affects
        ClassFamilyMask GetSpellAffectMask(uint32 spellId, SpellEffectIndex effectId) const
        {
            SpellAffectMap::const_iterator itr = mSpellAffectMap.find((spellId << 8) + effectId);
            if (itr != mSpellAffectMap.end())
                return ClassFamilyMask(itr->second);
            if (SpellEntry const* spellEntry = sSpellTemplate.LookupEntry<SpellEntry>(spellId))
                return ClassFamilyMask(spellEntry->EffectItemType[effectId]);
            return ClassFamilyMask();
        }

        SpellElixirMap const& GetSpellElixirMap() const { return mSpellElixirs; }

        uint32 GetSpellElixirMask(uint32 spellid) const
        {
            SpellElixirMap::const_iterator itr = mSpellElixirs.find(spellid);
            if (itr == mSpellElixirs.end())
                return 0x0;

            return itr->second;
        }

        SpellSpecific GetSpellElixirSpecific(uint32 spellid) const
        {
            uint32 mask = GetSpellElixirMask(spellid);
            if ((mask & ELIXIR_FLASK_MASK) == ELIXIR_FLASK_MASK)
                return SPELL_FLASK_ELIXIR;
            else if (mask & ELIXIR_BATTLE_MASK)
                return SPELL_BATTLE_ELIXIR;
            else if (mask & ELIXIR_GUARDIAN_MASK)
                return SPELL_GUARDIAN_ELIXIR;
            else if (mask & ELIXIR_WELL_FED)
                return SPELL_WELL_FED;
            else
                return SPELL_NORMAL;
        }

        SpellSpecific GetSpellFoodSpecific(const SpellEntry* entry) const
        {
            if (!entry)
                return SPELL_NORMAL;
            // Food / Drinks (mostly)
            if (entry->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                if (entry->SpellFamilyName == SPELLFAMILY_GENERIC)
                {
                    bool food = false;
                    bool drink = false;
                    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        switch (entry->EffectApplyAuraName[i])
                        {
                            // Food
                            case SPELL_AURA_MOD_REGEN:
                            case SPELL_AURA_OBS_MOD_HEALTH:
                                food = true;
                                break;
                            // Drink
                            case SPELL_AURA_MOD_POWER_REGEN:
                            case SPELL_AURA_OBS_MOD_MANA:
                                drink = true;
                                break;
                            default:
                                break;
                        }
                    }

                    if (food && drink)
                        return SPELL_FOOD_AND_DRINK;
                    else if (food)
                        return SPELL_FOOD;
                    else if (drink)
                        return SPELL_DRINK;
                }
            }
            // Well Fed buffs (must be exclusive with Food / Drink replenishment effects, or else Well Fed will cause them to be removed)
            else if (entry->HasAttribute(SPELL_ATTR_EX2_FOOD_BUFF))
                return SPELL_WELL_FED;

            // Spells without attributes, but classified as well fed
            // Multi-family check
            switch (entry->Id)
            {
                // Food buffs without attribute: instantly applied ones
                // Parent spell contains the attribute for them (TODO: add a query for parent spell in the future?)
                case 18125: // Blessed Sunfruit
                case 18141: // Blessed Sunfruit Juice
                case 18191: // Windblossom Berries
                case 18192: // Grilled Squid
                case 18193: // Marsh Lichen
                case 22730: // Runn Tum Tuber Surprise
                case 25661: // Dirge's Kickin' Chimaerok Chops
                case 46687: // Juicy Bear Burger
                // Alcohol: instant application, no attribute
                case 5020:  // Stormstout
                case 5021:  // Trogg Ale
                case 5257:  // Thunderbrew Lager
                case 5909:  // Watered-down Beer
                case 6114:  // Raptor Punch
                case 8553:  // Barleybrew Scalder
                case 20875: // Rumsey Rum
                case 22789: // Gordok Green Grog
                case 22790: // Kreeg's Stout Beatdown
                case 25037: // Rumsey Rum Light
                case 25722: // Rumsey Rum Dark
                case 25804: // Rumsey Rum Black Label
                case 37058: // Halaani Whiskey
                    return SPELL_WELL_FED;
            }
            return SPELL_NORMAL;
        }

        SpellThreatEntry const* GetSpellThreatEntry(uint32 spellid) const
        {
            SpellThreatMap::const_iterator itr = mSpellThreatMap.find(spellid);
            if (itr != mSpellThreatMap.end())
                return &itr->second;

            return nullptr;
        }

        float GetSpellThreatMultiplier(SpellEntry const* spellInfo) const
        {
            if (!spellInfo)
                return 1.0f;

            if (SpellThreatEntry const* entry = GetSpellThreatEntry(spellInfo->Id))
                return entry->multiplier;

            return 1.0f;
        }

        // Spell proc events
        SpellProcEventEntry const* GetSpellProcEvent(uint32 spellId) const
        {
            SpellProcEventMap::const_iterator itr = mSpellProcEventMap.find(spellId);
            if (itr != mSpellProcEventMap.end())
                return &itr->second;
            return nullptr;
        }

        // Spell procs from item enchants
        float GetItemEnchantProcChance(uint32 spellid) const
        {
            SpellProcItemEnchantMap::const_iterator itr = mSpellProcItemEnchantMap.find(spellid);
            if (itr == mSpellProcItemEnchantMap.end())
                return 0.0f;

            return itr->second;
        }

        static bool IsSpellProcEventCanTriggeredBy(SpellProcEventEntry const* spellProcEvent, uint32 EventProcFlag, SpellEntry const* procSpell, uint32 procFlags, uint32 procExtra);

        // Spell bonus data
        SpellBonusEntry const* GetSpellBonusData(uint32 spellId) const
        {
            // Lookup data
            SpellBonusMap::const_iterator itr = mSpellBonusMap.find(spellId);
            if (itr != mSpellBonusMap.end())
                return &itr->second;

            return nullptr;
        }

        // Spell target coordinates
        SpellTargetPosition const* GetSpellTargetPosition(uint32 spell_id) const
        {
            SpellTargetPositionMap::const_iterator itr = mSpellTargetPositions.find(spell_id);
            if (itr != mSpellTargetPositions.end())
                return &itr->second;
            return nullptr;
        }

        // Spell ranks chains
        SpellChainNode const* GetSpellChainNode(uint32 spell_id) const
        {
            SpellChainMap::const_iterator itr = mSpellChains.find(spell_id);
            if (itr == mSpellChains.end())
                return nullptr;

            return &itr->second;
        }

        uint32 GetFirstSpellInChain(uint32 spell_id) const
        {
            if (SpellChainNode const* node = GetSpellChainNode(spell_id))
                return node->first;

            return spell_id;
        }

        uint32 GetPrevSpellInChain(uint32 spell_id) const
        {
            if (SpellChainNode const* node = GetSpellChainNode(spell_id))
                return node->prev;

            return 0;
        }

        SpellChainMapNext const& GetSpellChainNext() const { return mSpellChainsNext; }

        template<typename Worker>
        void doForHighRanks(uint32 spellid, Worker& worker)
        {
            SpellChainMapNext const& nextMap = GetSpellChainNext();
            for (SpellChainMapNext::const_iterator itr = nextMap.lower_bound(spellid); itr != nextMap.upper_bound(spellid); ++itr)
            {
                worker(itr->second);
                doForHighRanks(itr->second, worker);
            }
        }

        // Note: not use rank for compare to spell ranks: spell chains isn't linear order
        // Use IsHighRankOfSpell instead
        uint8 GetSpellRank(uint32 spell_id) const
        {
            if (SpellChainNode const* node = GetSpellChainNode(spell_id))
                return node->rank;

            return 0;
        }

        bool IsHighRankOfSpell(uint32 spell1, uint32 spell2) const
        {
            SpellChainMap::const_iterator itr = mSpellChains.find(spell1);

            uint32 rank2 = GetSpellRank(spell2);

            // not ordered correctly by rank value
            if (itr == mSpellChains.end() || !rank2 || itr->second.rank <= rank2)
                return false;

            // check present in same rank chain
            for (; itr != mSpellChains.end(); itr = mSpellChains.find(itr->second.prev))
                if (itr->second.prev == spell2)
                    return true;

            return false;
        }

        bool IsRankSpellDueToSpell(SpellEntry const* spellInfo_1, uint32 spellId_2) const;
        bool IsNoStackSpellDueToSpell(SpellEntry const* spellInfo_1, SpellEntry const* spellInfo_2) const;
        bool canStackSpellRanksInSpellBook(SpellEntry const* spellInfo) const;
        bool IsRankedSpellNonStackableInSpellBook(SpellEntry const* spellInfo) const
        {
            return !canStackSpellRanksInSpellBook(spellInfo) && GetSpellRank(spellInfo->Id) != 0;
        }

        // return true if spell1 can affect spell2
        bool IsSpellCanAffectSpell(SpellEntry const* spellInfo_1, SpellEntry const* spellInfo_2) const;

        SpellEntry const* SelectAuraRankForLevel(SpellEntry const* spellInfo, uint32 Level) const;

        // Spell learning
        SpellLearnSkillNode const* GetSpellLearnSkill(uint32 spell_id) const
        {
            SpellLearnSkillMap::const_iterator itr = mSpellLearnSkills.find(spell_id);
            if (itr != mSpellLearnSkills.end())
                return &itr->second;
            else
                return nullptr;
        }

        bool IsSpellLearnSpell(uint32 spell_id) const
        {
            return mSpellLearnSpells.find(spell_id) != mSpellLearnSpells.end();
        }

        SpellLearnSpellMapBounds GetSpellLearnSpellMapBounds(uint32 spell_id) const
        {
            return mSpellLearnSpells.equal_range(spell_id);
        }

        bool IsSpellLearnToSpell(uint32 parent, uint32 child) const
        {
            SpellLearnSpellMapBounds bounds = GetSpellLearnSpellMapBounds(parent);
            for (SpellLearnSpellMap::const_iterator i = bounds.first; i != bounds.second; ++i)
                if (i->second.spell == child)
                    return true;
            return false;
        }

        static bool IsProfessionOrRidingSpell(uint32 spellId);
        static bool IsProfessionSpell(uint32 spellId);
        static bool IsPrimaryProfessionSpell(uint32 spellId);
        bool IsPrimaryProfessionFirstRankSpell(uint32 spellId) const;

        bool IsSkillBonusSpell(uint32 spellId) const;

        // Spell correctness for client using
        static bool IsSpellValid(SpellEntry const* spellInfo, Player* pl = nullptr, bool msg = true);

        SkillLineAbilityMapBounds GetSkillLineAbilityMapBounds(uint32 spell_id) const
        {
            return mSkillLineAbilityMap.equal_range(spell_id);
        }

        SkillRaceClassInfoMapBounds GetSkillRaceClassInfoMapBounds(uint32 skill_id) const
        {
            return mSkillRaceClassInfoMap.equal_range(skill_id);
        }

        PetAura const* GetPetAura(uint32 spell_id)
        {
            SpellPetAuraMap::const_iterator itr = mSpellPetAuraMap.find(spell_id);
            if (itr != mSpellPetAuraMap.end())
                return &itr->second;
            else
                return nullptr;
        }

        SpellCastResult GetSpellAllowedInLocationError(SpellEntry const* spellInfo, uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player = nullptr) const;

        SpellAreaMapBounds GetSpellAreaMapBounds(uint32 spell_id) const
        {
            return mSpellAreaMap.equal_range(spell_id);
        }

        SpellAreaForAuraMapBounds GetSpellAreaForAuraMapBounds(uint32 spell_id) const
        {
            return mSpellAreaForAuraMap.equal_range(spell_id);
        }

        SpellAreaForAreaMapBounds GetSpellAreaForAreaMapBounds(uint32 area_id) const
        {
            return mSpellAreaForAreaMap.equal_range(area_id);
        }

        // Modifiers
    public:
        static SpellMgr& Instance();

        void CheckUsedSpells(char const* table) const;

        // Loading data at server startup
        void LoadSpellChains();
        void LoadSpellLearnSkills();
        void LoadSpellLearnSpells();
        void LoadSpellScriptTarget();
        void LoadSpellAffects();
        void LoadSpellElixirs();
        void LoadSpellProcEvents();
        void LoadSpellProcItemEnchant();
        void LoadSpellBonuses();
        void LoadSpellTargetPositions();
        void LoadSpellThreats();
        void LoadSkillLineAbilityMap();
        void LoadSkillRaceClassInfoMap();
        void LoadSpellPetAuras();
        void LoadSpellAreas();

    private:
        SpellChainMap      mSpellChains;
        SpellChainMapNext  mSpellChainsNext;
        SpellLearnSkillMap mSpellLearnSkills;
        SpellLearnSpellMap mSpellLearnSpells;
        SpellTargetPositionMap mSpellTargetPositions;
        SpellAffectMap     mSpellAffectMap;
        SpellElixirMap     mSpellElixirs;
        SpellThreatMap     mSpellThreatMap;
        SpellProcEventMap  mSpellProcEventMap;
        SpellProcItemEnchantMap mSpellProcItemEnchantMap;
        SpellBonusMap      mSpellBonusMap;
        SkillLineAbilityMap mSkillLineAbilityMap;
        SkillRaceClassInfoMap mSkillRaceClassInfoMap;
        SpellPetAuraMap     mSpellPetAuraMap;
        SpellAreaMap         mSpellAreaMap;
        SpellAreaForAuraMap  mSpellAreaForAuraMap;
        SpellAreaForAreaMap  mSpellAreaForAreaMap;
};

#define sSpellMgr SpellMgr::Instance()
#endif
