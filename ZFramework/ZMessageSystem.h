#pragma once
#include "ZTypes.h"
#include <string>
#include <list>
#include <map>
#include <set>
#include <mutex>
#include <assert.h>




///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ZMessage
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Format:
// {
// [type=]type	(required)
// target = target	(optional)
// key1 = value1	(optional)
// ...
// keyn = valuen	(optional)
// }
//
// As a packed string:
// type=type;key1=val1...keyn=valn
class ZMessage
{
    friend class ZMessageSystem;
public:
	ZMessage();
    ZMessage(const std::string& sType, const std::string& sRawMessage, class IMessageTarget* pTarget = nullptr);
    ZMessage(const std::string& sRaw, class IMessageTarget* pTarget = nullptr);
    ZMessage(const ZMessage& rhs);

	~ZMessage();









	// Helper functions
    std::string     GetTarget() const { return mTarget; }
	void            SetTarget(const std::string& sTarget) { mTarget = sTarget; }

    std::string     GetType() const { return mType; }
    void            SetType(const std::string& sType) { mType = sType; }

	bool            HasParam(const std::string& sKey) const;
    std::string     GetParam(const std::string& sKey) const;
	void            SetParam(const std::string& sKey, const std::string& sVal);

	void            FromString(const std::string& sMessage);
    std::string     ToString() const;

    operator std::string() const { return ToString(); }

private:
	tKeyValueMap    mKeyValueMap;
    std::string     mTarget;
    std::string     mType;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMessageTarget
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class IMessageTarget
{
public:
	virtual std::string GetTargetName() = 0;							// returns a unique target identifier
	virtual bool        ReceiveMessage(const ZMessage& message) = 0;		// returns true if the message was handled.
};


typedef std::list<ZMessage>                  	    tMessageList;
typedef std::set<IMessageTarget*>           	    tMessageTargetSet;
typedef std::map<std::string, tMessageTargetSet> 	tMessageToMessageTargetsMap;
typedef std::map<std::string, IMessageTarget*>	    tNameToMessageTargetMap;


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// cCEMessageSystem
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZMessageSystem
{
public:
	ZMessageSystem();
	~ZMessageSystem();

	void    Process();

	void    RegisterTarget(IMessageTarget* pTarget);
	void    UnregisterTarget(IMessageTarget* pTarget);
    bool    IsRegistered(const std::string& sTargetName);

	void    AddNotification(const std::string& sMessageType, IMessageTarget* pTarget);
	void    RemoveNotification(const std::string& sMessageType, IMessageTarget* pTarget);
	void    RemoveAllNotifications(IMessageTarget* pTarget);

	void    Post(std::string sType, class IMessageTarget* pTarget = nullptr);
    void    Post(ZMessage& message, class IMessageTarget* pTarget = nullptr);






    template <typename T, typename...Types>
    void Post(std::string type, std::string key, T val, Types...more)
    {
        assert(type[0] == '{' && type[type.length() - 1] == '}');

        ZMessage m;
        m.mType = type.substr(1, type.length() - 2);
        ToMessage(m, key, val, more...);

        const std::lock_guard<std::mutex> lock(mMessageQueueMutex);
        mMessageQueue.push_back(std::move(m));
    }


    template <typename T, typename...Types>
    void Post(std::string type, IMessageTarget* pTarget, std::string key, T val, Types...more)
    {
        assert(type[0] == '{' && type[type.length()-1] == '}');

        ZMessage m;
        m.mType = type.substr(1,type.length()-2);

        if (pTarget)
            m.mTarget = pTarget->GetTargetName();
        ToMessage(m, key, val, more...);

        const std::lock_guard<std::mutex> lock(mMessageQueueMutex);
        mMessageQueue.push_back(std::move(m));
    }



    template <typename S, typename...SMore>
    inline void ToMessage(ZMessage& m, std::string key, S val, SMore...moreargs)
    {
        std::stringstream ss;
        ss << val;
        if (key == "target")
            m.mTarget = ss.str();
        else
            m.mKeyValueMap[key] = ss.str();
        return ToMessage(m, moreargs...);
    }

    inline void ToMessage(ZMessage&) {}   // needed for the variadic with no args








	// utility functions
    std::string                 GenerateUniqueTargetName();

private:

	tMessageList                mMessageQueue;
    std::mutex                  mMessageQueueMutex;

	tMessageToMessageTargetsMap mMessageToMessageTargetsMap;

	tNameToMessageTargetMap     mNameToMessageTargetMap;

	bool                        mbProccessing;
	std::atomic<int64_t>        mnUniqueTargetNameCount;
};

extern ZMessageSystem           gMessageSystem;
