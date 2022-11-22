#include "ZMessageSystem.h"
#include "helpers/StringHelpers.h"
#include "ZStdDebug.h"
#include "ZXMLNode.h"
#include <algorithm>


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

static int32_t gTotalMessageCount = 0;

ZMessage::ZMessage()
{
	gTotalMessageCount++;
}

ZMessage::ZMessage(const string& sMessage)
{
	gTotalMessageCount++;
	FromString(sMessage);
	ZASSERT_MESSAGE(HasParam("type"), string("Posted message:\"" + sMessage + "\" has no 'type' field.").c_str());
};

ZMessage::ZMessage(const ZMessage& message)
{
	gTotalMessageCount++;
	mKeyValueMap = message.mKeyValueMap;
}

ZMessage::~ZMessage()
{
	gTotalMessageCount--;
}



// Helper functions
bool ZMessage::HasParam(const string& sKey) const
{
	return mKeyValueMap.find(sKey) != mKeyValueMap.end();
}

string ZMessage::GetParam(const string& sKey) const
{
	tKeyValueMap::const_iterator it = mKeyValueMap.find(sKey);
	if (it != mKeyValueMap.end())
	{
		return (*it).second;
	}

	ZASSERT_MESSAGE(false, string("Key \"" + sKey + "\" not found!").c_str());
	return "";
}

void ZMessage::SetParam(const string& sKey, const string& sVal)
{
	mKeyValueMap[sKey] = sVal;
}

void ZMessage::FromString(const string& sMessage)
{
	mKeyValueMap.clear();

	// If the message has no 'msg=' then we treat the whole string as a single message
	if (ZXMLNode::FindSubstring(sMessage, "type=") == -1)
	{
		mKeyValueMap["type"] = sMessage;
	}
	else
	{
		string sParse(sMessage);

		bool bDone = false;
		while (!bDone)
		{
			string sPair;
			StringHelpers::SplitToken(sPair, sParse, ";");
			if (!sPair.empty())
			{
				string sKey;
                StringHelpers::SplitToken(sKey, sPair, "=");
				ZASSERT_MESSAGE(!sKey.empty(), "Key is empty!");
				ZASSERT_MESSAGE(!sPair.empty(), "Value is empty!");
				mKeyValueMap[sKey] = sPair;	// sPair now contains the value;
			}
			else
				bDone = true;
		}

		// Last key value pair
		if (!sParse.empty())
		{
			string sKey;
            StringHelpers::SplitToken(sKey, sParse, "=");
			ZASSERT_MESSAGE(!sKey.empty(), "Key is empty!");
			ZASSERT_MESSAGE(!sParse.empty(), "Value is empty!");
			mKeyValueMap[sKey] = sParse;	// sParse now contains the value;
		}
	}
}

string ZMessage::ToString() const
{
	string sRaw;
	for (tKeyValueMap::const_iterator it = mKeyValueMap.begin(); it != mKeyValueMap.end(); it++)
	{
		sRaw += (*it).first + "=" + (*it).second + ";";
	}

	return sRaw.substr(0, sRaw.length()-1);	// return everything but the final ';'
}








ZMessageSystem::ZMessageSystem()
{
	mnUniqueTargetNameCount = 0;
}

ZMessageSystem::~ZMessageSystem()
{
//	ZDEBUG_OUT("Messages in the end:%ld\n", gTotalMessageCount);
}

void ZMessageSystem::Process()
{
	// Messages can be posted from within a handler... 
	// so we only process the messages that are here when this is first called
	int64_t nNumMessagesToProcess = mMessageQueue.size();
	tMessageList::iterator it;

	while (nNumMessagesToProcess > 0)
	{
		ZMessage& msg = *mMessageQueue.begin();

		if (msg.HasTarget())
		{
            string sTarget(msg.GetTarget().c_str());
			tNameToMessageTargetMap::iterator it = mNameToMessageTargetMap.find(msg.GetTarget());
			//CEASSERT_MESSAGE(it != mNameToMessageTargetMap.end(), string("Target:\"" + msg.GetTarget() + "\" not registerred in message system!").c_str());
			if (it != mNameToMessageTargetMap.end())
			{
				((*it).second)->ReceiveMessage(msg);
			}
		}
		else
		{
			// Target-less message..... send to all listeners of that type
			tMessageTargetSet& messageTargets = mMessageToMessageTargetsMap[msg.GetType()];

			for (tMessageTargetSet::iterator targetIt = messageTargets.begin(); targetIt != messageTargets.end(); )
			{
				// We use an iterator to the next target... since the set can change in a callback
				tMessageTargetSet::iterator targetItNext = targetIt;
				targetItNext++;

				IMessageTarget* pTarget = *targetIt;
				ZASSERT(pTarget);

				pTarget->ReceiveMessage(msg);

				// If the HandleMessage call above cause the map to be cleared, break out of the loop
				if (mMessageToMessageTargetsMap.find(msg.GetType()) == mMessageToMessageTargetsMap.end())
					break;

				targetIt = targetItNext;
			}
		}

		mMessageQueue.pop_front();
		nNumMessagesToProcess--;
	}
}

void ZMessageSystem::AddNotification(const string& sMessageType, IMessageTarget* pTarget)
{
	mMessageToMessageTargetsMap[sMessageType].insert(pTarget);
}

void ZMessageSystem::RemoveNotification(const string& sMessageType, IMessageTarget* pTarget)
{
	tMessageTargetSet::iterator it = mMessageToMessageTargetsMap[sMessageType].find(pTarget);
	if (it == mMessageToMessageTargetsMap[sMessageType].end())
		return;

//	ZDEBUG_OUT("Removing Target - \"%s\" Message Type - \"%s\"\n", pTarget->GetTargetName().c_str(), sMessageType.c_str());
	mMessageToMessageTargetsMap[sMessageType].erase(it);

	if (mMessageToMessageTargetsMap[sMessageType].size() == 0)
	{
//		ZDEBUG_OUT("Message Type - \"%s\" has no more targets.\n", sMessageType.c_str());
		mMessageToMessageTargetsMap.erase(sMessageType);
	}
}

void ZMessageSystem::RemoveAllNotifications(IMessageTarget* pTarget)
{
//	CEASSERT( _CrtCheckMemory() == TRUE );
	for (tMessageToMessageTargetsMap::iterator messageTypeIt = mMessageToMessageTargetsMap.begin(); messageTypeIt != mMessageToMessageTargetsMap.end();)
	{
		tMessageToMessageTargetsMap::iterator messageTypeItNext = messageTypeIt;
		messageTypeItNext++;
		const string& sType = (*messageTypeIt).first;
		RemoveNotification(sType, pTarget);

		messageTypeIt = messageTypeItNext;
	}
//	CEASSERT( _CrtCheckMemory() == TRUE );
}

void ZMessageSystem::RegisterTarget(IMessageTarget* pTarget)
{
	ZASSERT(pTarget);
	ZASSERT_MESSAGE(mNameToMessageTargetMap.find(pTarget->GetTargetName()) == mNameToMessageTargetMap.end(), string("Target \"" + pTarget->GetTargetName() + "\" already mapped!").c_str());

	mNameToMessageTargetMap[pTarget->GetTargetName()] = pTarget;
}

void ZMessageSystem::UnregisterTarget(IMessageTarget* pTarget)
{
	ZASSERT(pTarget);
	tNameToMessageTargetMap::iterator it = mNameToMessageTargetMap.find(pTarget->GetTargetName());
	if (it != mNameToMessageTargetMap.end())
		mNameToMessageTargetMap.erase(it);
}


void ZMessageSystem::Post(string sRawMessages)
{
//	ZDEBUG_OUT("Post - \"%s\"\n", sRawMessages.c_str());
	// there can be multiple messages surrounded by "<msg>" and "</msg>"
	string sMessage;
	while (ZXMLNode::GetField(sRawMessages, "msg", sMessage))
	{
			ZMessage msg(sMessage);
			mMessageQueue.push_back(msg);
			sRawMessages = sRawMessages.substr((int) sMessage.length() + 11); // 5 for <msg> and 6 for </msg>
	}

	// post the last message
	if (!sRawMessages.empty())
	{
		if (sRawMessages.substr(0, 5) == "<msg>")
		{
			ZASSERT_MESSAGE(sRawMessages.substr(sRawMessages.length()-6) == "</msg", "Message delimiter does not end with \"</msg>\"");
			sRawMessages = sRawMessages.substr(5, sRawMessages.length()-11);	// -5 for <msg> and -6 for </msg>
		}
//		ZMessage msg(sRawMessages);
		mMessageQueue.emplace_back(sRawMessages);
	}
}

void ZMessageSystem::Post(ZMessage& message)
{
	mMessageQueue.push_back(std::move(message));
}

string ZMessageSystem::GenerateUniqueTargetName()
{
	return "target_" + StringHelpers::FromInt(mnUniqueTargetNameCount++);
}
