#include "ZMessageSystem.h"
#include "helpers/StringHelpers.h"
#include "ZDebug.h"
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

ZMessage::ZMessage(const string& sType, const string& sRawMessage, IMessageTarget* pTarget)
{
    mType = sType;
    assert(mType[0] != '{');

    gTotalMessageCount++;
    FromString(sRawMessage);
    if (pTarget)
        mTarget = pTarget->GetTargetName();
}

ZMessage::ZMessage(const std::string& sRaw, class IMessageTarget* pTarget)
{
    // new format requires all messages are enclosed with curly braces {}
    assert(!sRaw.empty() && sRaw[0] == '{' && sRaw[sRaw.length() - 1] == '}');
    
#ifdef _DEBUG
    size_t count = 0;
    for (auto c : sRaw)
        if (c == '{')
            count++;
    if (count > 1)
    {
        assert(false);  // Should not instantiate multi-message object.... needs to be posted to message system
    }
#endif


    if (sRaw.find(";") != string::npos)
    {
        FromString(sRaw);
    }
    else
    {
        mType = sRaw.substr(1, sRaw.length() - 2); // remove curlies
        assert(mType[0] != '{');
    }

    if (pTarget)
        mTarget = pTarget->GetTargetName();

    gTotalMessageCount++;
}



/*ZMessage::ZMessage(const string& sMessage)
{
	gTotalMessageCount++;
	FromString(sMessage);
	ZASSERT_MESSAGE(HasParam("type"), string("Posted message:\"" + sMessage + "\" has no 'type' field.").c_str());
};*/

ZMessage::ZMessage(const ZMessage& rhs)
{
	gTotalMessageCount++;
	mKeyValueMap = rhs.mKeyValueMap;
    mType = rhs.mType;
    mTarget = rhs.mTarget;
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
    assert(!sMessage.empty() && sMessage[0] == '{' && sMessage[sMessage.length() - 1] == '}');

	mKeyValueMap.clear();

	string sParse(sMessage.substr(1, sMessage.length()-2)); // strip curlies

    SH::SplitToken(mType, sParse, ";");  // first element
    assert(mType[0] != '{');

	bool bDone = false;
	while (!bDone)
	{
		string sPair;
		SH::SplitToken(sPair, sParse, ";");
		if (!sPair.empty())
		{
			string sKey;
            SH::SplitToken(sKey, sPair, "=");
			ZASSERT_MESSAGE(!sKey.empty(), "Key is empty!");
			ZASSERT_MESSAGE(!sPair.empty(), "Value is empty!");
            if (sKey == "target")
                mTarget = sPair;
            else if (sKey == "type")
            {
                mType = sPair;
                assert(mType[0] != '{');
            }
            else
                mKeyValueMap[sKey] = SH::URL_Decode(sPair);	// sParse now contains the value;
        }
		else
			bDone = true;
	}

	// Last key value pair
	if (!sParse.empty())
	{
		string sKey;
        SH::SplitToken(sKey, sParse, "=");
		ZASSERT_MESSAGE(!sKey.empty(), "Key is empty!");
		ZASSERT_MESSAGE(!sParse.empty(), "Value is empty!");
        if (sKey == "target")
            mTarget = sParse;
        else if (sKey == "type")
        {
            mType = sParse;
            assert(mType[0] != '{');
        }
        else
			mKeyValueMap[sKey] = SH::URL_Decode(sParse);	// sParse now contains the value;
	}
}

string ZMessage::ToString() const
{
    string sRaw( "{" + mType + ";");
    if (!mTarget.empty())
        sRaw += "target=" + mTarget + ";";

	for (tKeyValueMap::const_iterator it = mKeyValueMap.begin(); it != mKeyValueMap.end(); it++)
	{
		sRaw += (*it).first + "=" + SH::URL_Encode((*it).second) + ";";
	}

	return sRaw.substr(0, sRaw.length()-1) + "}";	// remove final ';' and add curly
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
    if (mMessageQueue.empty())
        return;

    mMessageQueueMutex.lock();
    tMessageList messages = std::move(mMessageQueue);
    mMessageQueueMutex.unlock();

	tMessageList::iterator it;

	for (auto& message : messages)
	{
        string sTarget(message.GetTarget());
		if (!sTarget.empty())
		{
			tNameToMessageTargetMap::iterator it = mNameToMessageTargetMap.find(sTarget);
			if (it != mNameToMessageTargetMap.end())
			{
                assert(!message.mType.empty());
				((*it).second)->ReceiveMessage(message);
			}
		}
		else
		{
			// Target-less message..... send to all listeners of that type
			tMessageTargetSet& messageTargets = mMessageToMessageTargetsMap[message.GetType()];

			for (tMessageTargetSet::iterator targetIt = messageTargets.begin(); targetIt != messageTargets.end(); )
			{
				// We use an iterator to the next target... since the set can change in a callback
				tMessageTargetSet::iterator targetItNext = targetIt;
				targetItNext++;

				IMessageTarget* pTarget = *targetIt;
				ZASSERT(pTarget);

				pTarget->ReceiveMessage(message);

				// If the HandleMessage call above cause the map to be cleared, break out of the loop
				if (mMessageToMessageTargetsMap.find(message.GetType()) == mMessageToMessageTargetsMap.end())
					break;

				targetIt = targetItNext;
			}
		}
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
	for (tMessageToMessageTargetsMap::iterator messageTypeIt = mMessageToMessageTargetsMap.begin(); messageTypeIt != mMessageToMessageTargetsMap.end();)
	{
		tMessageToMessageTargetsMap::iterator messageTypeItNext = messageTypeIt;
		messageTypeItNext++;
		const string& sType = (*messageTypeIt).first;
		RemoveNotification(sType, pTarget);

		messageTypeIt = messageTypeItNext;
	}
}

void ZMessageSystem::RegisterTarget(IMessageTarget* pTarget)
{
	ZASSERT(pTarget);
	ZASSERT_MESSAGE(IsRegistered(pTarget->GetTargetName()), string("Target \"" + pTarget->GetTargetName() + "\" already mapped!").c_str());

	mNameToMessageTargetMap[pTarget->GetTargetName()] = pTarget;
}

void ZMessageSystem::UnregisterTarget(IMessageTarget* pTarget)
{
	ZASSERT(pTarget);
	tNameToMessageTargetMap::iterator it = mNameToMessageTargetMap.find(pTarget->GetTargetName());
	if (it != mNameToMessageTargetMap.end())
		mNameToMessageTargetMap.erase(it);
}

bool ZMessageSystem::IsRegistered(const std::string& sTargetName)
{
    return mNameToMessageTargetMap.find(sTargetName) == mNameToMessageTargetMap.end();
}



void ZMessageSystem::Post(string sRaw, IMessageTarget* pTarget)
{
    if (sRaw.empty())
        return;

    if (sRaw[0] != '{')
    {
        assert(false);  // message must be enclosed with {}
        return;
    }



    // multiple messages to parse
    size_t messageStart = 0;
    size_t messageEnd = SH::FindMatching(sRaw, messageStart);
    while (messageStart != string::npos && messageEnd != string::npos)
    {
        string s(sRaw.substr(messageStart, messageEnd - messageStart+1));


        mMessageQueueMutex.lock();
        mMessageQueue.push_back(std::move(ZMessage(s, pTarget)));
        mMessageQueueMutex.unlock();

        messageStart = messageEnd;

        messageStart = sRaw.find('{', messageStart +1); // find a next message if there is one
        messageEnd = SH::FindMatching(sRaw, messageStart);
    }
}


void ZMessageSystem::Post(ZMessage& msg, IMessageTarget* pTarget)
{
    if (pTarget)
    {
        ZASSERT(!pTarget->GetTargetName().empty());
        msg.mTarget = pTarget->GetTargetName();
    }

    const std::lock_guard<std::mutex> lock(mMessageQueueMutex);
    mMessageQueue.push_back(std::move(msg));
}

string ZMessageSystem::GenerateUniqueTargetName()
{
	return "target_" + SH::FromInt(mnUniqueTargetNameCount++);
}

