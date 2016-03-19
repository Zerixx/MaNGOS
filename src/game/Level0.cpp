/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos-zero>
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "revision_nr.h"
#include "Util.h"
#include "BugReportMgr.h"

bool ChatHandler::HandleBugReportCount(char* /*args*/)
{
    size_t count = sBugReportMgr.GetBugReportList().size();

    PSendSysMessage("There are %lu bug reports.", count);

    return true;
}

bool ChatHandler::HandleBugReportListCommand(char* /*args*/)
{
    BugReportList list = sBugReportMgr.GetBugReportList();
    
    std::string playerName;
    
    size_t i;
    for (i = 0; i < list.size(); ++i)
    {
        sObjectMgr.GetPlayerNameByGUID(list[i].m_creator, playerName);
    
        struct tm* timeinfo = localtime(&list[i].m_date);
        char timeArr[25];
        strftime(timeArr, 25, "%F %T", timeinfo);
        
        PSendSysMessage("%lu - Creator: %s, Title: %s, Date: %s", i + 1, playerName.c_str(), list[i].m_title.c_str(), timeArr);
    }

    if (i == 0)
        PSendSysMessage("No bug reports exist!");

    return true;
}

bool ChatHandler::HandleBugReportShowCommand(char* args)
{
    uint32 index;
    ExtractUInt32(&args, index);
    --index;

    if (index >= sBugReportMgr.GetBugReportList().size())
    {
        PSendSysMessage("The given index was too large!");
        return false;
    }

    BugReport const& report = sBugReportMgr.GetBugReport(index);

    std::string playerName;
    sObjectMgr.GetPlayerNameByGUID(report.m_creator, playerName);

    struct tm* timeinfo = localtime(&report.m_date);
    char timeArr[25];
    strftime(timeArr, 25, "%F %T", timeinfo);

    PSendSysMessage("%u - Creator: %s, Title: %s, Date: %s\n%s", index + 1, playerName.c_str(), report.m_title.c_str(), timeArr, report.m_text.c_str());

    return true;
}

bool ChatHandler::HandleHelpCommand(char* args)
{
    if(!*args)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), args))
            SendSysMessage(LANG_NO_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(char* /*args*/)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleAccountCommand(char* args)
{
    // let show subcommands at unexpected data in args
    if (*args)
        return false;

    AccountTypes gmlevel = GetAccessLevel();
    PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmlevel));
    return true;
}

bool ChatHandler::HandleStartCommand(char* /*args*/)
{
    Player *chr = m_session->GetPlayer();

    if(chr->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    // cast spell Stuck
    chr->CastSpell(chr,7355,false);
    return true;
}

bool ChatHandler::HandleServerInfoCommand(char* /*args*/)
{
    uint32 activeClientsNum = sWorld.GetActiveSessionCount();
    uint32 queuedClientsNum = sWorld.GetQueuedSessionCount();
    uint32 maxQueuedClientsNum = sWorld.GetMaxQueuedSessionCount();
    std::string str = secsToTimeString(sWorld.GetUptime());

    // Get the maximum count for the last four weeks.
    uint32 maxActiveClientsNum;
    QueryResult* res = LoginDatabase.Query("SELECT MAX(maxplayers) FROM zp_realmd.uptime WHERE starttime > UNIX_TIMESTAMP(NOW()) - 2419000;");
    if (res)
    {
        Field* row = res->Fetch();
        maxActiveClientsNum = row[0].GetUInt32();

        delete res;
    }
    else
        maxActiveClientsNum = sWorld.GetMaxActiveSessionCount();

    char const* full;
    if(m_session)
        full = _FULLVERSION(REVISION_DATE,REVISION_TIME,REVISION_NR,"|cffffffff|Hurl:" REVISION_ID "|h" REVISION_ID "|h|r");
    else
        full = _FULLVERSION(REVISION_DATE,REVISION_TIME,REVISION_NR,REVISION_ID);
    SendSysMessage(full);

    if (sScriptMgr.IsScriptLibraryLoaded())
    {
        char const* ver = sScriptMgr.GetScriptLibraryVersion();
        if (ver && *ver)
            PSendSysMessage(LANG_USING_SCRIPT_LIB, ver);
        else
            SendSysMessage(LANG_USING_SCRIPT_LIB_UNKNOWN);
    }
    else
        SendSysMessage(LANG_USING_SCRIPT_LIB_NONE);

    PSendSysMessage(LANG_USING_WORLD_DB,sWorld.GetDBVersion());
    PSendSysMessage(LANG_USING_EVENT_AI,sWorld.GetCreatureEventAIVersion());
    PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
    PSendSysMessage(LANG_UPTIME, str.c_str());

    return true;
}

bool ChatHandler::HandleQuestMultiplierCommand(char* args)
{


    Player *chr = m_session->GetPlayer();

    if (!*args)
    {
        if(chr)
        {
            if(chr->GetQuestMultiplier() > 1)
            {
                PSendSysMessage("Your current quest EXP multiplier is set to %u. This is not the blizzlike setting. You can change the multiplier by using the command '.questexp 1-5'", chr->GetQuestMultiplier());
            }
            else
            {
                PSendSysMessage("Your current quest EXP multiplier is set to %u. This is the blizzlike setting. You can change the multiplier by using the command '.questexp 1-5'", chr->GetQuestMultiplier());
            }
        }
        
        return true;
    }

    if(args)
    {
        uint32 quest_exp = atoi(args);

        if(quest_exp < 1 || quest_exp > 3)
        {
            PSendSysMessage("Invalid value. You must use a value between 1 and 3, for example: '.questexp 2'.");
            return true;
        }
        
        if(chr)
        {
            chr->SetQuestMultiplier(quest_exp);

            CharacterDatabase.BeginTransaction();
            chr->SaveQuestExpRateToDB();
            CharacterDatabase.CommitTransaction();

            if(chr->GetQuestMultiplier() > 1)
            {
                PSendSysMessage("Your current quest EXP multiplier has been set to %u. This is not the blizzlike setting. You can change the multiplier by using the command '.questexp 1-5'", chr->GetQuestMultiplier());
            }
            else
            {
                PSendSysMessage("Your current quest EXP multiplier has been set to %u. This is the blizzlike setting. You can change the multiplier by using the command '.questexp 1-5'", chr->GetQuestMultiplier());
            }
        }
    }

    return true;
}

bool ChatHandler::HandleDismountCommand(char* /*args*/)
{
    //If player is not mounted, so go out :)
    if (!m_session->GetPlayer( )->IsMounted())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(m_session->GetPlayer( )->IsTaxiFlying())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->Unmount();
    m_session->GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    return true;
}

bool ChatHandler::HandleSaveCommand(char* /*args*/)
{
    Player *player=m_session->GetPlayer();

    // save GM account without delay and output message (testing, etc)
    if(GetAccessLevel() > SEC_PLAYER)
    {
        player->SaveToDB();
        SendSysMessage(LANG_PLAYER_SAVED);
        return true;
    }

    // save or plan save after 20 sec (logout delay) if current next save time more this value and _not_ output any messages to prevent cheat planning
    uint32 save_interval = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);
    if (save_interval==0 || (save_interval > 20*IN_MILLISECONDS && player->GetSaveTimer() <= save_interval - 20*IN_MILLISECONDS))
        player->SaveToDB();

    return true;
}

bool ChatHandler::HandleBGQueueCommand(char* args)
{
   char* px = ExtractLiteralArg(&args);

   // no arg
   if (!px)
      return false;

   Player* _player = m_session->GetPlayer();
   BattleGroundTypeId bgTypeId;
   ObjectGuid guid = _player->GetObjectGuid();

   // .bgqueue av
   if(strncmp(px,"av",3) == 0)
   {
      bgTypeId = BATTLEGROUND_AV; // 30
   }

   // .bgqueue ab
   else if(strncmp(px,"ab",3) == 0)
   {
      bgTypeId = BATTLEGROUND_AB; // 529
   }

    // .bgqueue wsg
   else if(strncmp(px,"wsg",4) == 0)
   {
      bgTypeId = BATTLEGROUND_WS; // 489
   }

   // invalid BG arg
   else
   {
      PSendSysMessage("Invalid battleground, command usage: .bgqueue [av/wsg/ab]");
      return false;
   }

   // Invalid level for BG
   if (!_player->GetBGAccessByLevel(bgTypeId)) {
      PSendSysMessage("You are too low level to access this battleground.");
        return false;
   }

   // Attempt to send BG List
   GetSession()->SendBattlegGroundList(guid, bgTypeId);

   return true;
}

bool ChatHandler::HandleGMListIngameCommand(char* /*args*/)
{
    std::list< std::pair<std::string, bool> > names;

    {
        HashMapHolder<Player>::ReadGuard g(HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType &m = sObjectAccessor.GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            AccountTypes itr_sec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->isGameMaster() || (itr_sec > SEC_PLAYER && itr_sec <= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_GM_LEVEL_IN_GM_LIST))) &&
                (!m_session || itr->second->IsVisibleGloballyFor(m_session->GetPlayer())))
                names.push_back(std::make_pair<std::string, bool>(GetNameLink(itr->second), itr->second->isAcceptWhispers()));
        }
    }

    if (!names.empty())
    {
        SendSysMessage(LANG_GMS_ON_SRV);

        char const* accepts = GetMangosString(LANG_GM_ACCEPTS_WHISPER);
        char const* not_accept = GetMangosString(LANG_GM_NO_WHISPER);
        for(std::list<std::pair< std::string, bool> >::const_iterator iter = names.begin(); iter != names.end(); ++iter)
            PSendSysMessage("%s - %s", iter->first.c_str(), iter->second ? accepts : not_accept);
    }
    else
        SendSysMessage(LANG_GMS_NOT_LOGGED);

    return true;
}

bool ChatHandler::HandleAccountPasswordCommand(char* args)
{
    // allow use from RA, but not from console (not have associated account id)
    if (!GetAccountId())
    {
        SendSysMessage (LANG_RA_ONLY_COMMAND);
        SetSentErrorMessage (true);
        return false;
    }

    // allow or quoted string with possible spaces or literal without spaces
    char *old_pass = ExtractQuotedOrLiteralArg(&args);
    char *new_pass = ExtractQuotedOrLiteralArg(&args);
    char *new_pass_c = ExtractQuotedOrLiteralArg(&args);

    if (!old_pass || !new_pass || !new_pass_c)
        return false;

    std::string password_old = old_pass;
    std::string password_new = new_pass;
    std::string password_new_c = new_pass_c;

    if (password_new != password_new_c)
    {
        SendSysMessage (LANG_NEW_PASSWORDS_NOT_MATCH);
        SetSentErrorMessage (true);
        return false;
    }

    if (!sAccountMgr.CheckPassword (GetAccountId(), password_old))
    {
        SendSysMessage (LANG_COMMAND_WRONGOLDPASSWORD);
        SetSentErrorMessage (true);
        return false;
    }

    AccountOpResult result = sAccountMgr.ChangePassword(GetAccountId(), password_new);

    switch(result)
    {
        case AOR_OK:
            SendSysMessage(LANG_COMMAND_PASSWORD);
            break;
        case AOR_PASS_TOO_LONG:
            SendSysMessage(LANG_PASSWORD_TOO_LONG);
            SetSentErrorMessage(true);
            return false;
        case AOR_NAME_NOT_EXIST:                            // not possible case, don't want get account name for output
        default:
            SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            SetSentErrorMessage(true);
            return false;
    }

    // OK, but avoid normal report for hide passwords, but log use command for anyone
    LogCommand(".account password *** *** ***");
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleAccountLockCommand(char* args)
{
    // allow use from RA, but not from console (not have associated account id)
    if (!GetAccountId())
    {
        SendSysMessage (LANG_RA_ONLY_COMMAND);
        SetSentErrorMessage (true);
        return false;
    }

    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    if (value)
    {
        LoginDatabase.PExecute( "UPDATE account SET locked = '1' WHERE id = '%u'", GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
    }
    else
    {
        LoginDatabase.PExecute( "UPDATE account SET locked = '0' WHERE id = '%u'", GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
    }

    return true;
}

/// Display the 'Message of the day' for the realm
bool ChatHandler::HandleServerMotdCommand(char* /*args*/)
{
    PSendSysMessage(LANG_MOTD_CURRENT, sWorld.GetMotd());
    return true;
}

bool ChatHandler::HandleRateCommand(char* args)
{
	uint32 rate;
	if (!ExtractUInt32(&args,rate))
		return false;

	if (rate == 1 || rate == 2)
	{
		Player *chr = m_session->GetPlayer();	
		chr->SetXPRate(rate);
		return true;
	}

	return false;
}
