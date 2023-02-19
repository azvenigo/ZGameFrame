#pragma once
#include "ZTypes.h"
#include <string>
#include <list>
#include <map>
#include <set>


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ZMessage
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Format:
// type = type	(required)
// target = target	(optional)
// key1 = value1	(optional)
// ...
// keyn = valuen	(optional)
//
// As a packed string:
// type=type;key1=val1...keyn=valn
class ZMessage
{
public:
	ZMessage();
	ZMessage(const std::string& sRawMessage);
	ZMessage(const ZMessage& message);

	~ZMessage();

	// Helper functions
    std::string     GetType() const { return GetParam("type"); }
	void            SetType(const std::string& sType) { SetParam("type", sType); }

	bool            HasTarget() const { return HasParam("target"); }
    std::string     GetTarget() const { return GetParam("target"); }
	void            SetTarget(const std::string& sTarget) { SetParam("target", sTarget); }

	bool            HasParam(const std::string& sKey) const;
    std::string     GetParam(const std::string& sKey) const;
	void            SetParam(const std::string& sKey, const std::string& sVal);

	void            FromString(const std::string& sMessage);
    std::string     ToString() const;

private:
	tKeyValueMap    mKeyValueMap;
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

	void							Process();

	void							RegisterTarget(IMessageTarget* pTarget);
	void							UnregisterTarget(IMessageTarget* pTarget);

	void							AddNotification(const std::string& sMessageType, IMessageTarget* pTarget);
	void							RemoveNotification(const std::string& sMessageType, IMessageTarget* pTarget);
	void							RemoveAllNotifications(IMessageTarget* pTarget);

	void							Post(ZMessage& message);
	void							Post(std::string sRawMessages);

	// utility functions
    std::string                     GenerateUniqueTargetName();

private:
	tMessageList					mMessageQueue;
	tMessageToMessageTargetsMap		mMessageToMessageTargetsMap;

	tNameToMessageTargetMap			mNameToMessageTargetMap;

	bool    						mbProccessing;
	int64_t							mnUniqueTargetNameCount;
};

extern ZMessageSystem				gMessageSystem;
