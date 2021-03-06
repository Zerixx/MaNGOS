/*
 * Copyright (C) 2006-2011 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2010-2011 ScriptDev0 <http://github.com/mangos-zero/scriptdev0>
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

/* ScriptData
SDName: Boss_Nefarian
SD%Complete: 80
SDComment: Some issues with class calls effecting more than one class
SDCategory: Blackwing Lair
EndScriptData */

#include "precompiled.h"
#include "blackwing_lair.h"
#include "TemporarySummon.h"

enum
{
    SAY_AGGRO                   = -1469007,
    SAY_XHEALTH                 = -1469008,             // at 5% hp
    SAY_SHADOWFLAME             = -1469009,
    SAY_RAISE_SKELETONS         = -1469010,
    SAY_SLAY                    = -1469011,
    SAY_DEATH                   = -1469012,

    SAY_MAGE                    = -1469013,
    SAY_WARRIOR                 = -1469014,
    SAY_DRUID                   = -1469015,
    SAY_PRIEST                  = -1469016,
    SAY_PALADIN                 = -1469017,
    SAY_SHAMAN                  = -1469018,
    SAY_WARLOCK                 = -1469019,
    SAY_HUNTER                  = -1469020,
    SAY_ROGUE                   = -1469021,

    SPELL_SHADOWFLAME_INITIAL   = 22992,                // old spell id 22972 -> wrong
    SPELL_SHADOWFLAME           = 22539,
    SPELL_BELLOWING_ROAR        = 22686,
    SPELL_VEIL_OF_SHADOW        = 22687,                // old spell id 7068 -> wrong
    SPELL_CLEAVE                = 20691,
    SPELL_TAIL_LASH             = 23364,
    SPELL_BONE_CONTRUST         = 23363,                //23362, 23361   Missing from DBC!

    SPELL_MAGE                  = 23410,                // wild magic
    SPELL_WARRIOR               = 23397,                // beserk
    SPELL_DRUID                 = 23398,                // cat form
    SPELL_PRIEST                = 23401,                // corrupted healing
    SPELL_PALADIN               = 23418,                // syphon blessing
    SPELL_SHAMAN                = 23425,                // totems
    SPELL_WARLOCK               = 23427,                // infernals    -> should trigger 23426
    SPELL_HUNTER                = 23436,                // bow broke
    SPELL_ROGUE                 = 23414,                // Paralise

    FACTION_BLACK_DRAGON        = 103
};

struct MANGOS_DLL_DECL boss_nefarianAI : public ScriptedAI
{
    boss_nefarianAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
		m_creature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_FIRE, true);
        hasLanded = false;
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiShadowFlameTimer;
    uint32 m_uiBellowingRoarTimer;
    uint32 m_uiVeilOfShadowTimer;
    uint32 m_uiCleaveTimer;
    uint32 m_uiTailLashTimer;
    uint32 m_uiClassCallTimer;
    uint32 m_landTimer;
    uint32 m_landEmoteTimer;
    bool m_bPhase3;
    bool m_bHasEndYell;
    bool hasLanded;

    void Reset()
    {
        m_uiShadowFlameTimer    = 12000;                            // These times are probably wrong
        m_uiBellowingRoarTimer  = 30000;
        m_uiVeilOfShadowTimer   = 15000;
        m_uiCleaveTimer         = 7000;
        m_uiTailLashTimer       = 10000;
        m_uiClassCallTimer      = 35000;                            // 35-40 seconds
        m_landTimer             = 7500;
        m_landEmoteTimer        = 0;
        m_bPhase3               = false;
        m_bHasEndYell           = false;
    
        if(hasLanded)
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
            m_creature->RemoveSplineFlag(SPLINEFLAG_WALKMODE);
            m_creature->SetHover(true);
            m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND/* | UNIT_BYTE1_FLAG_UNK_2*/);
            m_creature->SetSplineFlags(SPLINEFLAG_FLYING);
            m_creature->MonsterMove(-7443.177f, -1338.277f, 486.649f, 7500);
        }
    }

    void KilledUnit(Unit* pVictim)
    {
        if (urand(0, 4))
            return;

        DoScriptText(SAY_SLAY, m_creature, pVictim);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_NEFARIAN, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
        {
            m_pInstance->SetData(TYPE_NEFARIAN, FAIL);

            // Cleanup encounter
            if (m_creature->IsTemporarySummon())
            {
                TemporarySummon* pTemporary = (TemporarySummon*)m_creature;

                if (Creature* pNefarius = m_creature->GetMap()->GetCreature(pTemporary->GetSummonerGuid()))
                    pNefarius->AI()->ResetToHome();
            }

            m_creature->ForcedDespawn();
        }

        instance_blackwing_lair* blackwing_lair = dynamic_cast<instance_blackwing_lair*>(m_pInstance);
        if (blackwing_lair)
        {
            for (ObjectGuid current_spawn_guid : blackwing_lair->GetDrakonoidsAndBoneConstructs())
            {
                Creature* current_spawn = m_creature->GetMap()->GetCreature(current_spawn_guid);
                if (current_spawn)
                    ((TemporarySummon*) current_spawn)->UnSummon();
            }

            blackwing_lair->GetDrakonoidsAndBoneConstructs().clear();
        }

    }

    void Aggro(Unit* /*pWho*/)
    {
        DoScriptText(SAY_AGGRO, m_creature);

        // Remove flying in case Nefarian aggroes before his combat point was reached
        if (m_creature->HasSplineFlag(SPLINEFLAG_FLYING))
        {
            m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, 0);
            m_creature->RemoveSplineFlag(SPLINEFLAG_FLYING);
        }

        DoCast(m_creature, SPELL_SHADOWFLAME_INITIAL);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if(m_landTimer)
        {
            if(m_landTimer <= uiDiff)
            {
                m_creature->HandleEmote(EMOTE_ONESHOT_LAND);
                m_creature->SetHover(false);
                m_creature->SetSplineFlags(SPLINEFLAG_NONE);
               // m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, 0/* | UNIT_BYTE1_FLAG_UNK_2*/);
                m_landEmoteTimer = 3000;
                m_landTimer = 0;
            }
            else
                m_landTimer -= uiDiff;
        }

        if(m_landEmoteTimer)
        {
            if(m_landEmoteTimer <= uiDiff)
            {
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                m_creature->SetInCombatWithZone();
                if (Unit* pTarget = m_creature->SelectRandomUnfriendlyTarget(0, 100.0f))//>SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    AttackStart(pTarget);

                m_landEmoteTimer = 0;
                hasLanded = true;
            }
            else
                m_landEmoteTimer -= uiDiff;
        }

        for (auto pRef : m_creature->getThreatManager().getThreatList())
        {
           float x, y, z;
           Player* player = m_creature->GetMap()->GetPlayer(pRef->getUnitGuid());

           if (player)
           {
               player->GetPosition(x, y, z);

               if((player->GetDistanceSqr(-7532.82f, -1243.3f, 479.98f) < 36.0f)) // if (x > -7540.795898f && x < -7524.379883f && x <-7531.26f && x > -7533.93f && y < -1235.48f && y > -1251.88f && y < -1242.45f && y > -1245.19f)
                    player->NearTeleportTo(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, false);
           } 
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // ShadowFlame_Timer
        if (m_uiShadowFlameTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_SHADOWFLAME) == CAST_OK)
            {
                // ToDo: check if he yells at every cast
                DoScriptText(SAY_SHADOWFLAME, m_creature);
                m_uiShadowFlameTimer = 12000;
            }
        }
        else
            m_uiShadowFlameTimer -= uiDiff;

        // BellowingRoar_Timer
        if (m_uiBellowingRoarTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_BELLOWING_ROAR) == CAST_OK)
                m_uiBellowingRoarTimer = 30000;
        }
        else
            m_uiBellowingRoarTimer -= uiDiff;

        // VeilOfShadow_Timer
        if (m_uiVeilOfShadowTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_VEIL_OF_SHADOW) == CAST_OK)
                m_uiVeilOfShadowTimer = 15000;
        }
        else
            m_uiVeilOfShadowTimer -= uiDiff;

        // Cleave_Timer
        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = 7000;
        }
        else
            m_uiCleaveTimer -= uiDiff;

        // TailLash_Timer
        if (m_uiTailLashTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TAIL_LASH) == CAST_OK)
                m_uiTailLashTimer = 10000;
        }
        else
            m_uiTailLashTimer -= uiDiff;

        // ClassCall_Timer
        if (m_uiClassCallTimer < uiDiff && !m_creature->getThreatManager().getThreatList().empty())
        {
            std::vector<uint8> class_list;
            for (HostileReference* current_target_ref : m_creature->getThreatManager().getThreatList())
            {
                Player* current_target = dynamic_cast<Player*>(current_target_ref->getTarget());
                if (current_target && std::find(class_list.begin(), class_list.end(), current_target->getClass()) == class_list.end())
                    class_list.push_back(current_target->getClass());
            }

            switch(class_list[urand(0, class_list.size() - 1)])
            {
            case CLASS_MAGE:
                DoScriptText(SAY_MAGE, m_creature);
                m_creature->CastSpell(m_creature, SPELL_MAGE, true);	// Works
                break;
            case CLASS_WARRIOR:
                DoScriptText(SAY_WARRIOR, m_creature);
                m_creature->CastSpell(m_creature, SPELL_WARRIOR, true); // Works
                break;
            case CLASS_DRUID:
                DoScriptText(SAY_DRUID, m_creature);
                m_creature->CastSpell(m_creature, SPELL_DRUID, true);  // Works
                break;
            case CLASS_PRIEST:
                DoScriptText(SAY_PRIEST, m_creature);
                m_creature->CastSpell(m_creature, SPELL_PRIEST, true); // Works
                break;
            case CLASS_PALADIN:
                DoScriptText(SAY_PALADIN, m_creature);
                m_creature->CastSpell(m_creature, SPELL_PALADIN, true); // Works
                break;
            case CLASS_SHAMAN:
                DoScriptText(SAY_SHAMAN, m_creature);
                m_creature->CastSpell(m_creature, SPELL_SHAMAN, true); // Works
                break;
            case CLASS_WARLOCK:
                DoScriptText(SAY_WARLOCK, m_creature);
                m_creature->CastSpell(m_creature, SPELL_WARLOCK, true); // Works
                break;
            case CLASS_HUNTER:
                DoScriptText(SAY_HUNTER, m_creature);
                m_creature->CastSpell(m_creature, SPELL_HUNTER, true); // Works
                break;
            case CLASS_ROGUE:
                DoScriptText(SAY_ROGUE, m_creature);
                m_creature->CastSpell(m_creature, SPELL_ROGUE, true); // Works
                break;
            }

            m_uiClassCallTimer = urand(35000, 40000);
        }
        else
            m_uiClassCallTimer -= uiDiff;

        // Phase3 begins when we are below X health
        if (!m_bPhase3 && m_creature->GetHealthPercent() < 20.0f)
        {
            // revive all dead dragos as 14605
            instance_blackwing_lair* blackwing_lair = dynamic_cast<instance_blackwing_lair*>(m_pInstance);
            if (blackwing_lair)
            {
                std::vector<ObjectGuid> tmp_list;

                for (ObjectGuid current_drakonoid_guid : blackwing_lair->GetDrakonoidsAndBoneConstructs())
                {
                    Creature* current_drakonoid = m_creature->GetMap()->GetCreature(current_drakonoid_guid);
                    if (current_drakonoid)
                    {
                        float x, y, z;

                        current_drakonoid->GetPosition(x, y, z);

                        Creature* bone_construct = m_creature->SummonCreature(14605, x, y, z, 0.f, TEMPSUMMON_DEAD_DESPAWN, 0);

                        if (bone_construct)
                        {
                            bone_construct->setFaction(FACTION_BLACK_DRAGON);

                            Player* rnd_player = GetRandomPlayerInCurrentMap();
                            bone_construct->Attack(rnd_player ? rnd_player : m_creature->getVictim(), true);
                            bone_construct->GetMotionMaster()->MoveChase(rnd_player ? rnd_player : m_creature->getVictim());

                            tmp_list.push_back(bone_construct->GetObjectGuid());
                        }

                        ((TemporarySummon*) current_drakonoid)->UnSummon();
                    }
                }

                blackwing_lair->GetDrakonoidsAndBoneConstructs().clear();
                blackwing_lair->GetDrakonoidsAndBoneConstructs() = tmp_list;

            }


            m_bPhase3 = true;
            DoScriptText(SAY_RAISE_SKELETONS, m_creature);
        }

        // 5% hp yell
        if (!m_bHasEndYell && m_creature->GetHealthPercent() < 5.0f)
        {
            m_bHasEndYell = true;
            DoScriptText(SAY_XHEALTH, m_creature);
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_nefarian(Creature* pCreature)
{
    return new boss_nefarianAI(pCreature);
}

void AddSC_boss_nefarian()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_nefarian";
    pNewScript->GetAI = &GetAI_boss_nefarian;
    pNewScript->RegisterSelf();
}
