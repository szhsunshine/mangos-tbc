/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
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

/* ScriptData
SDName: Boss_Arlokk
SD%Complete: 80
SDComment: Vanish spell is replaced by workaround; Timers
SDCategory: Zul'Gurub
EndScriptData */

#include "AI/ScriptDevAI/include/precompiled.h"
#include "zulgurub.h"

 /* ContentData
boss_arlokk
npc_zulian_prowler
go_gong_of_bethekk
EndContentData */

enum
{
    SAY_AGGRO                   = -1309011,
    SAY_FEAST_PANTHER           = -1309012,
    SAY_DEATH                   = -1309013,

    SPELL_SHADOW_WORD_PAIN      = 23952,
    SPELL_GOUGE                 = 24698,
    SPELL_MARK_ARLOKK           = 24210,
    SPELL_RAVAGE                = 24213,
    SPELL_TRASH                 = 3391,
    SPELL_WHIRLWIND             = 24236,
    SPELL_PANTHER_TRANSFORM     = 24190,
    SPELL_SUMMON_ZULIAN_PROWLERS = 24247,

    SPELL_SNEAK                  = 22766,
};

struct boss_arlokkAI : public ScriptedAI
{
    boss_arlokkAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (instance_zulgurub*)pCreature->GetInstanceData();
        Reset();
    }

    instance_zulgurub* m_pInstance;

    uint32 m_uiShadowWordPainTimer;
    uint32 m_uiGougeTimer;
    uint32 m_uiMarkTimer;
    uint32 m_uiRavageTimer;
    uint32 m_uiTrashTimer;
    uint32 m_uiWhirlwindTimer;
    uint32 m_uiVanishTimer;
    uint32 m_uiVisibleTimer;
    uint32 m_uiTransformTimer;
    uint32 m_uiSummonTimer;
    Creature* m_pTrigger1;
    Creature* m_pTrigger2;

    GuidList m_lProwlerGUIDList;

    bool m_bIsPhaseTwo;

    void Reset() override
    {
        m_uiShadowWordPainTimer = 8000;
        m_uiGougeTimer      = 14000;
        m_uiMarkTimer       = 5000;
        m_uiRavageTimer     = 12000;
        m_uiTrashTimer      = 20000;
        m_uiWhirlwindTimer  = 15000;
        m_uiTransformTimer  = 30000;
        m_uiVanishTimer     = 5000;
        m_uiVisibleTimer    = 0;

        m_bIsPhaseTwo = false;

        // Restore visibility
        if (m_creature->GetVisibility() != VISIBILITY_ON)
            m_creature->SetVisibility(VISIBILITY_ON);
    }

    void Aggro(Unit* /*pWho*/) override
    {
        DoScriptText(SAY_AGGRO, m_creature);
        if (m_pTrigger1 = m_pInstance->SelectRandomPantherTrigger(true))
            m_pTrigger1->CastSpell(m_pTrigger1, SPELL_SUMMON_ZULIAN_PROWLERS, TRIGGERED_NONE);
        if (m_pTrigger2 = m_pInstance->SelectRandomPantherTrigger(false))
            m_pTrigger2->CastSpell(m_pTrigger2, SPELL_SUMMON_ZULIAN_PROWLERS, TRIGGERED_NONE);
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_ARLOKK, FAIL);

        // we should be summoned, so despawn
        m_creature->ForcedDespawn();

        DoStopZulianProwlers();
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        DoScriptText(SAY_DEATH, m_creature);
        // Restore visibility in case of killed by dots
        if (m_creature->GetVisibility() != VISIBILITY_ON)
            m_creature->SetVisibility(VISIBILITY_ON);

        DoStopZulianProwlers();

        if (m_pInstance)
            m_pInstance->SetData(TYPE_ARLOKK, DONE);
    }

    // Wrapper to despawn the zulian panthers on evade / death
    void DoStopZulianProwlers()
    {
        if (m_pInstance)
        {
            // Stop summoning Zulian prowlers
            if (m_pTrigger1)
                m_pTrigger1->RemoveAurasDueToSpell(SPELL_SUMMON_ZULIAN_PROWLERS);
            if (m_pTrigger2)
                m_pTrigger2->RemoveAurasDueToSpell(SPELL_SUMMON_ZULIAN_PROWLERS);
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiVisibleTimer)
        {
            if (m_uiVisibleTimer <= uiDiff)
            {
                // Restore visibility
                m_creature->SetVisibility(VISIBILITY_ON);

                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    AttackStart(pTarget);

                m_uiVisibleTimer = 0;
            }
            else
                m_uiVisibleTimer -= uiDiff;

            // Do nothing while vanished
            return;
        }

        // Troll phase
        if (!m_bIsPhaseTwo)
        {
            if (m_uiShadowWordPainTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_SHADOW_WORD_PAIN) == CAST_OK)
                        m_uiShadowWordPainTimer = 15000;
                }
            }
            else
                m_uiShadowWordPainTimer -= uiDiff;

            if (m_uiMarkTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_MARK_ARLOKK, SELECT_FLAG_PLAYER))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_MARK_ARLOKK) == CAST_OK)
                    {
                        DoScriptText(SAY_FEAST_PANTHER, m_creature, pTarget);
                        m_uiMarkTimer = 30000;
                    }
                }
            }
            else
                m_uiMarkTimer -= uiDiff;

            if (m_uiGougeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GOUGE) == CAST_OK)
                {
                    if (m_creature->getThreatManager().getThreat(m_creature->getVictim()))
                        m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(), -80);

                    m_uiGougeTimer = urand(17000, 27000);
                }
            }
            else
                m_uiGougeTimer -= uiDiff;

            // Transform to Panther
            if (m_uiTransformTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_PANTHER_TRANSFORM) == CAST_OK)
                {
                    m_uiTransformTimer = 80000;
                    m_bIsPhaseTwo = true;
                }
            }
            else
                m_uiTransformTimer -= uiDiff;
        }
        // Panther phase
        else
        {
            if (m_uiRavageTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_RAVAGE) == CAST_OK)
                    m_uiRavageTimer = urand(10000, 15000);
            }
            else
                m_uiRavageTimer -= uiDiff;

            if (m_uiTrashTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRASH) == CAST_OK)
                    m_uiTrashTimer = urand(13000, 15000);
            }
            else
                m_uiTrashTimer -= uiDiff;

            if (m_uiWhirlwindTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_WHIRLWIND) == CAST_OK)
                    m_uiWhirlwindTimer = 15000;
            }
            else
                m_uiWhirlwindTimer -= uiDiff;

            if (m_uiVanishTimer < uiDiff)
            {
                // Note: this is a workaround because we do not know the real vanish spell
                m_creature->SetVisibility(VISIBILITY_OFF);
                DoResetThreat();

                m_uiVanishTimer = 85000;
                m_uiVisibleTimer = 45000;
            }
            else
                m_uiVanishTimer -= uiDiff;

            // Transform back
            if (m_uiTransformTimer < uiDiff)
            {
                m_creature->RemoveAurasDueToSpell(SPELL_PANTHER_TRANSFORM);
                m_uiTransformTimer = 30000;
                m_bIsPhaseTwo = false;
            }
            else
                m_uiTransformTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

struct npc_zulian_prowlerAI : public ScriptedAI
{
    npc_zulian_prowlerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_bMoveToAid = m_creature->IsTemporarySummon();
        Reset();
    }

    GuidList m_lProwlerGUIDList;

    bool m_bMoveToAid;

    void Reset() override
    {
        DoCastSpellIfCan(m_creature, SPELL_SNEAK);

        // Do only once, and only for those summoned by Arlokk
        if (m_bMoveToAid)
        {
            // Add to GUID list to despawn later
            m_lProwlerGUIDList.push_back(m_creature->GetObjectGuid());

            // Count the number of prowlers alive
            uint32 count = 0;
            for (GuidList::const_iterator itr = m_lProwlerGUIDList.begin(); itr != m_lProwlerGUIDList.end(); ++itr)
            {
                if (Unit* pProwler = m_creature->GetMap()->GetUnit(*itr))
                    if (pProwler->isAlive())
                        count++;
            }

            // Check if more than 40 are alive, if so, despawn
            if (count > 40)
            {
                m_creature->ForcedDespawn();
                return;
            }

            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MovePoint(1, aArlokkWallShieldPos[0], aArlokkWallShieldPos[1], aArlokkWallShieldPos[2]);

            m_bMoveToAid = false;
        }
    }
};

CreatureAI* GetAI_boss_arlokk(Creature* pCreature)
{
    return new boss_arlokkAI(pCreature);
}

CreatureAI* GetAI_npc_zulian_prowler(Creature* pCreature)
{
    return new npc_zulian_prowlerAI(pCreature);
}

bool GOUse_go_gong_of_bethekk(Player* /*pPlayer*/, GameObject* pGo)
{
    if (ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData())
    {
        if (pInstance->GetData(TYPE_ARLOKK) == DONE || pInstance->GetData(TYPE_ARLOKK) == IN_PROGRESS)
            return true;

        pInstance->SetData(TYPE_ARLOKK, IN_PROGRESS);
    }

    return false;
}

void AddSC_boss_arlokk()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_arlokk";
    pNewScript->GetAI = &GetAI_boss_arlokk;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_zulian_prowler";
    pNewScript->GetAI = &GetAI_npc_zulian_prowler;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "go_gong_of_bethekk";
    pNewScript->pGOUse = &GOUse_go_gong_of_bethekk;
    pNewScript->RegisterSelf();
}
