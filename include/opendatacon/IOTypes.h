/*	opendatacon
 *
 *	Copyright (c) 2015:
 *
 *		DCrip3fJguWgVCLrZFfA7sIGgvx1Ou3fHfCxnrz4svAi
 *		yxeOtDhDCXf1Z4ApgXvX5ahqQmzRfJ2DoX8S05SqHA==
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */
/*
 * IOTypes.h
 *
 *  Created on: 20/12/2015
 *      Author: Alan Murray <alan@atmurray.net>
 */

#ifndef IOTYPES_H_
#define IOTYPES_H_

#include <chrono>
#include <string>
#include <tuple>

#include <opendatacon/EnumClassFlags.h>

namespace odc
{

//enumerate all the different type of events that can pass through opendatacon
//As a starting point, define values to correspond with the previously used dnp3 measurment and output types, and some extras
// that way it should be easy to migrate from using actual opendnp3 library types
enum class EventType: uint8_t
{
	//Used to start iteration
	//Must be first value
	BeforeRange               = 0,

	//Inputs
	Binary                    = 1,
	DoubleBitBinary           = 2,
	Analog                    = 3,
	Counter                   = 4,
	FrozenCounter             = 5,
	BinaryOutputStatus        = 6,
	AnalogOutputStatus        = 7,
	BinaryCommandEvent        = 8,
	AnalogCommandEvent        = 9,
	OctetString               = 10,
	TimeAndInterval           = 11,
	SecurityStat              = 12,

	Reserved1                 = 13,
	Reserved2                 = 14,
	Reserved3                 = 15,

	//Outputs
	ControlRelayOutputBlock   = 16,
	AnalogOutputInt16         = 17,
	AnalogOutputInt32         = 18,
	AnalogOutputFloat32       = 19,
	AnalogOutputDouble64      = 20,

	Reserved4                 = 21,
	Reserved5                 = 22,
	Reserved6                 = 23,

	//Quality (for when the quality changes, but not the value)
	BinaryQuality             = 24,
	DoubleBitBinaryQuality    = 25,
	AnalogQuality             = 26,
	CounterQuality            = 27,
	BinaryOutputStatusQuality = 28,
	FrozenCounterQuality      = 29,
	AnalogOutputStatusQuality = 30,

	Reserved7                 = 31,
	Reserved8                 = 32,
	Reserved9                 = 33,

	//File Control
	FileAuth                  = 34,
	FileCommand               = 35,
	FileCommandStatus         = 36,
	FileTransport             = 37,
	FileTransportStatus       = 38,
	FileDescriptor            = 39,
	FileSpecString            = 40,

	Reserved10                = 41,
	Reserved11                = 42,
	Reserved12                = 43,

	//Connection events
	ConnectState              = 44,

	//Used to end iteration
	//Must be last value
	AfterRange                = 45
};
constexpr EventType operator +( const EventType lhs, const int rhs)
{
	return static_cast<EventType>(static_cast<const uint8_t>(lhs) + rhs);
}

enum class CommandStatus : uint8_t
{
	SUCCESS = 0,
	TIMEOUT = 1,
	NO_SELECT = 2,
	FORMAT_ERROR = 3,
	NOT_SUPPORTED = 4,
	ALREADY_ACTIVE = 5,
	HARDWARE_ERROR = 6,
	LOCAL = 7,
	TOO_MANY_OPS = 8,
	NOT_AUTHORIZED = 9,
	AUTOMATION_INHIBIT = 10,
	PROCESSING_LIMITED = 11,
	OUT_OF_RANGE = 12,
	DOWNSTREAM_LOCAL = 13,
	ALREADY_COMPLETE = 14,
	BLOCKED = 15,
	CANCELLED = 16,
	BLOCKED_OTHER_MASTER = 17,
	DOWNSTREAM_FAIL = 18,
	NON_PARTICIPATING = 126,
	UNDEFINED = 127
};

enum class ControlCode : uint8_t
{
	NUL = 1,
	NUL_CANCEL = 2,
	PULSE_ON = 3,
	PULSE_ON_CANCEL = 4,
	PULSE_OFF = 5,
	PULSE_OFF_CANCEL = 6,
	LATCH_ON = 7,
	LATCH_ON_CANCEL = 8,
	LATCH_OFF = 9,
	LATCH_OFF_CANCEL = 10,
	CLOSE_PULSE_ON = 11,
	CLOSE_PULSE_ON_CANCEL = 12,
	TRIP_PULSE_ON = 13,
	TRIP_PULSE_ON_CANCEL = 14,
	UNDEFINED = 15
};

#define ENUMSTRING(A,E,B) if(A == E::B) return #B;
inline std::string ToString(const ControlCode cc)
{
	ENUMSTRING(cc,ControlCode,NUL                  )
	ENUMSTRING(cc,ControlCode,NUL_CANCEL           )
	ENUMSTRING(cc,ControlCode,PULSE_ON             )
	ENUMSTRING(cc,ControlCode,PULSE_ON_CANCEL      )
	ENUMSTRING(cc,ControlCode,PULSE_OFF            )
	ENUMSTRING(cc,ControlCode,PULSE_OFF_CANCEL     )
	ENUMSTRING(cc,ControlCode,LATCH_ON             )
	ENUMSTRING(cc,ControlCode,LATCH_ON_CANCEL      )
	ENUMSTRING(cc,ControlCode,LATCH_OFF            )
	ENUMSTRING(cc,ControlCode,LATCH_OFF_CANCEL     )
	ENUMSTRING(cc,ControlCode,CLOSE_PULSE_ON       )
	ENUMSTRING(cc,ControlCode,CLOSE_PULSE_ON_CANCEL)
	ENUMSTRING(cc,ControlCode,TRIP_PULSE_ON        )
	ENUMSTRING(cc,ControlCode,TRIP_PULSE_ON_CANCEL )
	ENUMSTRING(cc,ControlCode,UNDEFINED            )
	return "UNKNOWN";
}

//Quatilty flags that can be used for any EventType
//Start with a superset of all the dnp3 type qualities
enum class QualityFlags: uint16_t
{
	NONE              = 0,
	ONLINE            = 1<<0,
	RESTART           = 1<<1,
	COMM_LOST         = 1<<2,
	REMOTE_FORCED     = 1<<3,
	LOCAL_FORCED      = 1<<4,
	OVERRANGE         = 1<<5,
	REFERENCE_ERR     = 1<<6,
	ROLLOVER          = 1<<7,
	DISCONTINUITY     = 1<<8,
	CHATTER_FILTER    = 1<<9
};
ENABLE_BITWISE(QualityFlags)

#define FLAGSTRING(E,X) if((q & E::X) == E::X) s += #X "|";
inline std::string ToString(const QualityFlags q)
{
	std::string s = "|";
	FLAGSTRING(QualityFlags,ONLINE        )
	FLAGSTRING(QualityFlags,RESTART       )
	FLAGSTRING(QualityFlags,COMM_LOST     )
	FLAGSTRING(QualityFlags,REMOTE_FORCED )
	FLAGSTRING(QualityFlags,LOCAL_FORCED  )
	FLAGSTRING(QualityFlags,OVERRANGE     )
	FLAGSTRING(QualityFlags,REFERENCE_ERR )
	FLAGSTRING(QualityFlags,ROLLOVER      )
	FLAGSTRING(QualityFlags,DISCONTINUITY )
	FLAGSTRING(QualityFlags,CHATTER_FILTER)
	if(s == "|") return "|NONE|";
	return s;
}

struct ControlRelayOutputBlock
{
	ControlCode functionCode = ControlCode::LATCH_ON;
	uint8_t count = 1;
	uint32_t onTimeMS = 100;
	uint32_t offTimeMS = 100;
	CommandStatus status = CommandStatus::SUCCESS;

	explicit operator std::string() const
	{
		return ToString(functionCode)+"|Count "+std::to_string(count)+
		       "|ON "+std::to_string(onTimeMS)+"ms|OFF "+std::to_string(offTimeMS)+"ms)";
	}
};

enum class ConnectState {PORT_UP,CONNECTED,DISCONNECTED,PORT_DOWN};

typedef uint64_t msSinceEpoch_t;
inline msSinceEpoch_t msSinceEpoch()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
		       (std::chrono::system_clock::now().time_since_epoch()).count();
}

//Map EventTypes to payload types
template<EventType t> struct EventTypePayload { typedef void type; };
#define EVENTPAYLOAD(E,T)\
	template<> struct EventTypePayload<E>{ typedef T type; };

//TODO: make these structs?
typedef std::pair<bool,bool> DBB;
typedef std::tuple<msSinceEpoch_t,uint32_t,uint8_t> TAI;
typedef std::pair<uint16_t,uint32_t> SS;
typedef std::pair<int16_t,CommandStatus> AO16;
typedef std::pair<int32_t,CommandStatus> AO32;
typedef std::pair<float,CommandStatus> AOF;
typedef std::pair<double,CommandStatus> AOD;

EVENTPAYLOAD(EventType::Binary                   , bool)
EVENTPAYLOAD(EventType::DoubleBitBinary          , DBB)
EVENTPAYLOAD(EventType::Analog                   , double)
EVENTPAYLOAD(EventType::Counter                  , uint32_t)
EVENTPAYLOAD(EventType::FrozenCounter            , uint32_t)
EVENTPAYLOAD(EventType::BinaryOutputStatus       , bool)
EVENTPAYLOAD(EventType::AnalogOutputStatus       , double)
EVENTPAYLOAD(EventType::BinaryCommandEvent       , CommandStatus)
EVENTPAYLOAD(EventType::AnalogCommandEvent       , CommandStatus)
EVENTPAYLOAD(EventType::OctetString              , std::string)
EVENTPAYLOAD(EventType::TimeAndInterval          , TAI)
EVENTPAYLOAD(EventType::SecurityStat             , SS)
EVENTPAYLOAD(EventType::ControlRelayOutputBlock  , ControlRelayOutputBlock)
EVENTPAYLOAD(EventType::AnalogOutputInt16        , AO16)
EVENTPAYLOAD(EventType::AnalogOutputInt32        , AO32)
EVENTPAYLOAD(EventType::AnalogOutputFloat32      , AOF)
EVENTPAYLOAD(EventType::AnalogOutputDouble64     , AOD)
EVENTPAYLOAD(EventType::BinaryQuality            , QualityFlags)
EVENTPAYLOAD(EventType::DoubleBitBinaryQuality   , QualityFlags)
EVENTPAYLOAD(EventType::AnalogQuality            , QualityFlags)
EVENTPAYLOAD(EventType::CounterQuality           , QualityFlags)
EVENTPAYLOAD(EventType::BinaryOutputStatusQuality, QualityFlags)
EVENTPAYLOAD(EventType::FrozenCounterQuality     , QualityFlags)
EVENTPAYLOAD(EventType::AnalogOutputStatusQuality, QualityFlags)
EVENTPAYLOAD(EventType::ConnectState             , ConnectState)
//TODO: map the rest


class EventInfo
{
public:
	EventInfo(EventType tp, size_t ind = 0, const std::string& source = "",
		QualityFlags qual = (QualityFlags::ONLINE | QualityFlags::RESTART),
		msSinceEpoch_t time = msSinceEpoch()):
		Index(ind),
		Timestamp(time),
		Quality(qual),
		SourcePort(source),
		Type(tp)
	{}

	//Getters
	const EventType& GetEventType() const { return Type; }
	const size_t& GetIndex() const { return Index; }
	const msSinceEpoch_t& GetTimestamp() const { return Timestamp; }
	const QualityFlags& GetQuality() const { return Quality; }
	const std::string& GetSourcePort() const { return SourcePort; }

	template<EventType t>
	const typename EventTypePayload<t>::type& GetPayload() const
	{
		if(t != Type)
			throw std::runtime_error("Wrong payload type requested for selected odc::EventInfo");
		return Payload<t>();
	}

	//Setters
	void SetIndex(size_t i){ Index = i; }
	void SetTimestamp(msSinceEpoch_t tm = msSinceEpoch()){ Timestamp = tm; }
	void SetQuality(QualityFlags q){ Quality = q; }
	void SetSource(const std::string& s){ SourcePort = s; }

	template<EventType t>
	void SetPayload(typename EventTypePayload<t>::type&& p)
	{
		if(t != Type)
			throw std::runtime_error("Wrong payload type specified for selected odc::EventInfo");
		Payload<t>() = std::move(p);
	}

private:
	size_t Index;
	msSinceEpoch_t Timestamp;
	QualityFlags Quality;
	std::string SourcePort;
	const EventType Type;

	template<EventType t>
	typename EventTypePayload<t>::type& Payload() const
	{
		static typename EventTypePayload<t>::type Payload;
		return Payload;
	}
};

}

#endif
