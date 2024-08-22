#pragma once
#include "ZTypes.h"
#include <string>
#include <list>
#include <map>
#include <set>
#include <mutex>
#include <assert.h>
#include <iostream>


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMessageTarget
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class IMessageTarget
{
public:
    virtual std::string GetTargetName() = 0;                                // returns a unique target identifier
    virtual bool        ReceiveMessage(const class ZMessage& message) = 0;  // returns true if the message was handled.
};

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
//    ZMessage(const std::string& sType, const std::string& sRawMessage, class IMessageTarget* pTarget = nullptr);
//    ZMessage(const std::string& sRaw, class IMessageTarget* pTarget = nullptr);

    ZMessage(std::string sRaw)
    {
        FromString(sRaw);
    };

    ZMessage(const ZMessage& rhs);

	~ZMessage();




    // Fallback converter (slower than specialized below)
    template <typename T>
    std::string ConvertToString(const T& val) 
    {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }
  
    template <>
    std::string ConvertToString<int64_t>(const int64_t& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<uint64_t>(const uint64_t& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<int32_t>(const int32_t& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<uint32_t>(const uint32_t& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<float>(const float& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<double>(const double& val) {
        return std::to_string(val);
    }
    template <>
    std::string ConvertToString<std::string>(const std::string& val) {
        return val;
    }
    template <>
    std::string ConvertToString<const char*>(const char* const& val) {
        return std::string(val);
    }

    



    template <typename T, typename...Types>
    ZMessage(const std::string& type, const std::string& key, T val, Types...more)
    {
        mType = type;
        ToMessage(key, val, more...);
    }

    ZMessage(const std::string& type, class IMessageTarget* pTarget)
    {
        mType = type;
        assert(pTarget);
        mTarget = pTarget->GetTargetName();
    }


    template <typename T, typename...Types>
    ZMessage(const std::string& type, class IMessageTarget* pTarget, std::string key, T val, Types...more)
    {
        mType = type;
        assert(pTarget);
        mTarget = pTarget->GetTargetName();
        ToMessage(key, val, more...);
    }

    template <typename S, typename...SMore>
    inline void ToMessage(const std::string& key, S val, SMore...moreargs)
    {
        if (key == "target")
            mTarget = val;
        else
            mKeyValueMap[key] = ConvertToString(val);

        return ToMessage(moreargs...);
    }

    inline void ToMessage() {}   // needed for the variadic with no args


















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

    void    Post(const std::string& sRawMessage);
    void    Post(const ZMessage& message);



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
