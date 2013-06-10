/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "sysdefs.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"

#include "exec.h"

#include "mblsyntax.h"
#include "mblsensor.h"
//#include "mblad.h"


////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

static const char *s_orientation_names[] =
{
	"unknown", "portrait", "portrait upside down", "landscape right", "landscape left", "face up", "face down", nil
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCanComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    MCTextMessagingGetCanComposeTextMessage(ctxt, t_result);
    
    if (t_result)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
    if (!ctxt . HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleComposeTextMessage(void *p_context, MCParameter *p_parameters)
{
    MCAutoStringRef t_recipients, t_body;
    
    bool t_success;
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	t_success = MCParseParameters(p_parameters, "x", &t_recipients);
    if (t_success == false)
    {
         ctxt . SetTheResultToValue(kMCFalse);
        return ES_NORMAL;
    }
	t_success = MCParseParameters(p_parameters, "x", &t_body);
    
    ep . clear();
  
    
    if (t_success)
        MCTextMessagingExecComposeTextMessage(ctxt, *t_recipients, *t_body);
    
	return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleLockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecLockIdleTimer(ctxt);
    
    return ES_NORMAL;
}


Exec_stat MCHandleUnlockIdleTimer(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCIdleTimerExecUnlockIdleTimer(ctxt);
    
    return ES_NORMAL;    
}

Exec_stat MCHandleIdleTimerLocked(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCIdleTimerGetIdleTimerLocked(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);    
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;        
    else
        return ES_NORMAL;
}


///////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleCanMakePurchase(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_result;
    
    MCStoreGetCanMakePurchase(ctxt, t_result);
    
    if (t_result)
        ctxt.SetTheResultToValue(kMCTrue);
    else
        ctxt.SetTheResultToValue(kMCFalse);
    
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;    
}

Exec_stat MCHandleEnablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecEnablePurchaseUpdates(ctxt);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandleDisablePurchaseUpdates(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecDisablePurchaseUpdates(ctxt);
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}
    
Exec_stat MCHandleRestorePurchases(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCStoreExecRestorePurchases(ctxt);
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}


Exec_stat MCHandlePurchaseList(void* p_context, MCParameter* p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    MCAutoStringRef t_list;
    
    MCStoreGetPurchaseList(ctxt, &t_list);
    
    
    if(ctxt.HasError())
        return ES_ERROR;
    else
    {
        ctxt.SetTheResultToValue(*t_list);
        return ES_NORMAL;
    }
}

Exec_stat MCHandlePurchaseCreate(void* p_context, MCParameter* p_parameters)
{
    bool t_success = true;
    MCAutoStringRef t_product_id;
    uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &t_product_id);
    
    if (t_success)
        MCStoreExecCreatePurchase(ctxt, &t_product_id, t_id);
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToNumber(t_id);
        return ES_NORMAL;
    }
}


Exec_stat MCHandlePurchaseState(void* p_context, MCParameter* p_parameters)
{    
	bool t_success = true;
	
	uint32_t t_id;
    MCAutoStringRef t_state;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
	if (t_success)
		MCStoreGetPurchaseState(ctxt, t_id, &t_state);
	
	if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;}
    else
    {
        ctxt.SetTheResultToValue(*t_state);
        return ES_NORMAL;
    }
}


Exec_stat MCHandlePurchaseError(void* p_context, MCParameter* p_parameters)
{
	bool t_success = true;
	
    MCStringRef t_error;
	uint32_t t_id;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
	if (t_success)
        MCStoreGetPurchaseError(ctxt, t_id, t_error);
    
    if (ctxt.HasError())
    {
        ctxt.SetTheResultToEmpty();
        return ES_ERROR;
    }
    else
    {
        ctxt.SetTheResultToValue(t_error);
        return ES_NORMAL;
    }
}

Exec_stat MCHandlePurchaseGet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "us", &t_id, &t_prop_name);
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	
	if (t_success)
        MCStoreGetPurchaseProperty(ctxt, t_id, &t_prop_name);
	
	if (!ctxt . HasError())
    {
        MCAutoStringRef t_string;
        /* UNCHECKED */ ep . copyasstringref(&t_string);
		ctxt . SetTheResultToValue(*t_string);
        return ES_NORMAL;
    }
    
    ctxt . SetTheResultToEmpty();
	return ES_ERROR;
}


Exec_stat MCHandlePurchaseSet(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	MCAutoStringRef t_prop_name;
    uint32_t t_quantity;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "uxu", &t_id, &t_prop_name, &t_quantity);
		
	MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreSetPurchaseProperty(ctxt, t_id, &t_prop_name, t_quantity);
	
    if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}



Exec_stat MCHandlePurchaseSendRequest(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecSendPurchaseRequest(ctxt, t_id);
    
	if (ctxt.HasError())
        return ES_ERROR;
    else
        return ES_NORMAL;
}

Exec_stat MCHandlePurchaseConfirmDelivery(void *context, MCParameter *p_parameters)
{
	bool t_success = true;
	
	uint32_t t_id;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "u", &t_id);
	
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
	if (t_success)
        MCStoreExecConfirmPurchaseDelivery(ctxt, t_id);
    
    if (ctxt.HasError())
        return ES_ERROR;
	else
        return ES_NORMAL;
}


Exec_stat MCHandleRequestProductDetails(void *context, MCParameter *p_parameters)
{
    MCAutoStringRef t_product;
    bool t_success = true;    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "x", &t_product);
        
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecRequestProductDetails(ctxt, *t_product);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

Exec_stat MCHandlePurchaseVerify(void *context, MCParameter *p_parameters)
{
    bool t_success = true;
    bool t_verified = true;
    
    uint32_t t_id;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "ub", &t_id, &t_verified);
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    if (t_success)
        MCStoreExecPurchaseVerify(ctxt, t_id, t_verified);
    
    if (!ctxt.HasError())
        return ES_NORMAL;
    
    return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetDeviceOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intenum_t t_orientation;
	MCOrientationGetOrientation(ctxt, t_orientation);

	ctxt . SetTheResultToStaticCString(s_orientation_names[(int)t_orientation]);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	intset_t t_orientations;
	MCOrientationGetAllowedOrientations(ctxt, t_orientations);

	bool t_success;
	t_success = true;
	
	MCAutoListRef t_orientation_list;

	if (t_success)
		t_success = MCListCreateMutable(EC_COMMA, &t_orientation_list);

	for (uint32_t j = 0; s_orientation_names[j] != nil; j++)
	{
		if ((t_orientations & (1 << j)) != 0)
		{		
			MCAutoStringRef t_orientation;
			if (t_success)
				t_success = MCStringFormat(&t_orientation, "%s", s_orientation_names[j]);

			if (t_success)
				t_success = MCListAppend(*t_orientation_list, *t_orientation);
		}
	}

	MCAutoStringRef t_result;
	if (t_success)
		t_success = MCListCopyAsString(*t_orientation_list, &t_result);

	if (t_success)
	{
		ctxt . SetTheResultToValue(*t_result);
		return ES_NORMAL;
	}

	return ES_ERROR;
}

Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCAutoStringRef t_orientations;
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		ep . copyasstringref(&t_orientations);
	}

    bool t_success = true;
	char **t_orientations_array;
	uint32_t t_orientations_count;
	t_orientations_array = nil;
	t_orientations_count = 0;
	if (t_success)
		t_success = MCCStringSplit(MCStringGetCString(*t_orientations), ',', t_orientations_array, t_orientations_count);
	
	intset_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
    {
		for(uint32_t i = 0; i < t_orientations_count; i++)
        {
            if (MCCStringEqualCaseless(t_orientations_array[i], "portrait"))
                t_orientations_set |= ORIENTATION_PORTRAIT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "portrait upside down"))
                t_orientations_set |= ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "landscape right"))
                t_orientations_set |= ORIENTATION_LANDSCAPE_RIGHT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "landscape left"))
                t_orientations_set |= ORIENTATION_LANDSCAPE_LEFT_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "face up"))
                t_orientations_set |= ORIENTATION_FACE_UP_BIT;
            else if (MCCStringEqualCaseless(t_orientations_array[i], "face down"))
                t_orientations_set |= ORIENTATION_FACE_DOWN_BIT;
        }
	}
    
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);

	MCOrientationSetAllowedOrientations(ctxt, t_orientations_set);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_locked;
	MCOrientationGetOrientationLocked(ctxt, t_locked);

    if (t_locked)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCOrientationExecLockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	MCOrientationExecUnlockOrientation(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	MCAutoStringRef t_address, t_cc_address, t_subject, t_message_body;

	MCExecPoint ep(nil, nil, nil);
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_cc_address);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_subject);
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		/* UNCHECKED */ ep . copyasstringref(&t_message_body);
		p_parameters = p_parameters -> getnext();
	}
	
	MCExecContext ctxt(ep);

	MCMailExecSendEmail(ctxt, *t_address, *t_cc_address, *t_subject, *t_message_body);
	
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposePlainMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
		MCMailExecComposeUnicodeMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_to, t_cc, t_bcc, t_subject, t_body;
	MCAutoArrayRef t_attachments;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "|xxxxxa", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	if (t_success)
        MCMailExecComposeHtmlMail(ctxt, *t_to, *t_cc, *t_bcc, *t_subject, *t_body, *t_attachments);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	MCExecContext ctxt(ep);

	bool t_can_send;

	MCMailGetCanSendMail(ctxt, t_can_send);
    
    if (t_can_send)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleStartTrackingSensor(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_loosely = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStartTrackingSensor(ctxt, (intenum_t)t_sensor, t_loosely);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleStopTrackingSensor(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)t_sensor);
    }
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneEnableAcceleromter etc)
Exec_stat MCHandleAccelerometerEnablement(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeAcceleration, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeAcceleration);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleLocationTrackingState(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeLocation, false);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeLocation);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingTrackingState(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(ctxt, kMCSensorTypeHeading, true);
    else
        MCSensorExecStopTrackingSensor(ctxt, (intenum_t)kMCSensorTypeHeading);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorReading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_detailed = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCAutoStringRef t_reading;

    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
        {
            if (t_detailed)
                MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetLocationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeHeading:
        {
            if (t_detailed)
                MCSensorGetDetailedHeadingOfDevice(ctxt,& t_detailed_reading);
            else
                MCSensorGetHeadingOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            if (t_detailed)
                MCSensorGetDetailedAccelerationOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetAccelerationOfDevice(ctxt, &t_reading);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            if (t_detailed)
                MCSensorGetDetailedRotationRateOfDevice(ctxt, &t_detailed_reading);
            else
                MCSensorGetRotationRateOfDevice(ctxt, &t_reading);
            break;
        }
        default:
            break;
    }
    
    if (t_detailed)
    {
        if (*t_detailed_reading != nil)
            ep.setvalueref(*t_detailed_reading);
    }
    else
    {
        if (*t_reading != nil)
            ep.setvalueref(*t_reading);
    }
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

// MM-2012-02-11: Added support old style sensor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedLocationOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ep.setvalueref(*t_detailed_reading);
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCurrentHeading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    MCAutoArrayRef t_detailed_reading;
    MCSensorGetDetailedHeadingOfDevice(ctxt, &t_detailed_reading);
    if (*t_detailed_reading != nil)
        ep.setvalueref(*t_detailed_reading);
    
	MCAutoStringRef t_result;
	ep . copyasstringref(&t_result);
    ctxt . SetTheResultToValue(*t_result);
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSetHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_timeout = atoi(ep.getcstring());
    }
    MCSensorSetLocationCalibrationTimeout(ctxt, t_timeout);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibrationTimeout(ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);
    
    ctxt . SetTheResultToEmpty();
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleSensorAvailable(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }    
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, t_sensor, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackLocation(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
        
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeLocation, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCanTrackHeading(void *p_context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(ctxt, kMCSensorTypeHeading, t_available);
    
    if (t_available)
        ctxt . SetTheResultToValue(kMCTrue);
    else
        ctxt . SetTheResultToValue(kMCFalse);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

bool MCContactParseParams(MCParameter *p_params, MCArrayRef &r_contact, char *&r_title, char *&r_message, char *&r_alternate_name)
{
	bool t_success = true;
	
	char *t_title = nil;
	char *t_message = nil;
	char *t_alternate_name = nil;
	
	t_success = MCParseParameters(p_params, "a|sss", r_contact, &t_title, &t_message, &t_alternate_name);
	
	if (t_success)
	{
		r_title = t_title;
		r_message = t_message;
		r_alternate_name = t_alternate_name;
	}
	else
	{
		MCCStringFree(t_title);
		MCCStringFree(t_message);
		MCCStringFree(t_alternate_name);
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandlePickContact(void *context, MCParameter *p_parameters) // ABPeoplePickerNavigationController
{
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

    MCAddressBookExecPickContact(ctxt);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleShowContact(void *context, MCParameter *p_parameters) // ABPersonViewController
{
    int32_t t_contact_id = 0;
    int32_t r_result;
    MCExecPoint ep(nil, nil, nil);

    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);

    MCAddressBookExecShowContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleCreateContact(void *context, MCParameter *p_parameters) // ABNewPersonViewController
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

    MCAddressBookExecCreateContact(ctxt);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleUpdateContact(void *context, MCParameter *p_parameters) // ABUnknownPersonViewController
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);

	MCAutoArrayRef t_contact;
	MCAutoStringRef t_title;
	MCAutoStringRef t_message;
	MCAutoStringRef t_alternate_name;

	if (MCParseParameters(p_parameters, "axxx", &t_contact, &t_title, &t_message, &t_alternate_name))
	    MCAddressBookExecUpdateContact(ctxt, *t_contact, *t_title, *t_message, *t_alternate_name);
    
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleGetContactData(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);

	MCAutoArrayRef t_contact_data;
	MCAddressBookGetContactData(ctxt, t_contact_id, &t_contact_data);

	if (*t_contact_data != nil)
		ctxt . SetTheResultToValue(*t_contact_data);
	else
		ctxt . SetTheResultToEmpty();

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleRemoveContact(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);

    int32_t t_contact_id = 0;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_id = atoi (ep.getsvalue().getstring());
    }

    MCExecContext ctxt(ep);
    
	MCAddressBookExecRemoveContact(ctxt, t_contact_id);

	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleAddContact(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    // Handle parameters. We are doing that in a dedicated call
	MCAutoArrayRef t_contact;
	
	/* UNCHECKED */ MCParseParameters(p_parameters, "a", &(&t_contact));

    MCExecContext ctxt(ep);
    // Call the Exec implementation
    MCAddressBookExecAddContact(ctxt, *t_contact);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}

Exec_stat MCHandleFindContact(void *context, MCParameter *p_parameters)
{
    const char *t_contact_name = NULL;
    const char *r_result = NULL;
    MCExecPoint ep(nil, nil, nil);
	ep . clear();
    // Handle parameters.
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_contact_name = ep.getcstring();
    }
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    // Call the Exec implementation
    //MCAddressBookExecFindContact(ctxt, t_contact_name);
    // Set return value
	if (!ctxt . HasError())
		return ES_NORMAL;

	return ES_ERROR;
}


//////////////////////////////////////////////////////////////////////////////


Exec_stat MCHandleAdRegister(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_key;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &t_key);
	
	if (t_success)
		MCAdExecRegisterWithInneractive(ctxt, *t_key);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
    return ES_ERROR;
}


Exec_stat MCHandleAdCreate(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    MCAutoStringRef t_type;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xx", &t_ad, &t_type);
    
//    MCAdTopLeft t_top_left = {0,0};
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;

    if (t_success)
    {
//        char *t_top_left_string;
//        t_top_left_string = nil;
//        if (MCParseParameters(p_parameters, "s", &t_top_left_string))
//        /* UNCHECKED */ sscanf(t_top_left_string, "%u,%u", &t_top_left.x, &t_top_left.y);
//        MCCStringFree(t_top_left_string);
        t_success = MCParseParameters(p_parameters, "uu", &t_topleft_x, &t_topleft_y);
    }
    
    MCAutoArrayRef t_metadata;
    
    if (t_success)
        MCParseParameters(p_parameters, "a", &t_metadata);
    
	if (t_success)
		MCAdExecCreateAd(ctxt, *t_ad, *t_type, t_topleft_x, t_topleft_y, *t_metadata);
    
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdDelete(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;

	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
	if (t_success)
		MCAdExecDeleteAd(ctxt, *t_ad);
        
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdGetVisible(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
    bool t_visible;
    t_visible = false;
    
	if (t_success)
		MCAdGetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    if (t_success)
        MCresult->sets(MCU_btos(t_visible));
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdSetVisible(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    
    bool t_visible;
    t_visible = false;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "xb", &t_ad, &t_visible);
	
	if (t_success)
		MCAdSetVisibleOfAd(ctxt, *t_ad, t_visible);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdGetTopLeft(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();

	MCAutoStringRef t_ad;
    
	if (t_success)
		t_success = MCParseParameters(p_parameters, "x", &t_ad);
	
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;
    
	if (t_success)
		MCAdGetTopLeftOfAd(ctxt, *t_ad, t_topleft_x, t_topleft_y);
    
#ifdef MOBILE_BROKEN
    if (t_success)
    {
        MCAutoRawCString t_top_left_string;
        t_success = MCCStringFormat(t_top_left_string, "%u,%u", t_top_left.x, t_top_left.y);
        if (t_success)
            if (t_top_left_string.Borrow() != nil)
                ep.copysvalue(t_top_left_string.Borrow());
    }
    
    if (t_success)
        MCresult->store(ep, False);
#endif
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAdSetTopLeft(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
    
	MCAutoStringRef t_ad;
    uint32_t t_topleft_x;
    uint32_t t_topleft_y;
	
    if (t_success)
		t_success = MCParseParameters(p_parameters, "suu", &t_ad, t_topleft_x, t_topleft_y);
    
	if (t_success)
		MCAdSetTopLeftOfAd(ctxt, *t_ad, t_topleft_x, t_topleft_y);
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandleAds(void *context, MCParameter *p_parameters)
{
    bool t_success;
    t_success = true;
    
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
	ctxt . SetTheResultToEmpty();
#ifdef OLD_EXEC
    if (t_success)
    {
        MCAutoRawCString t_ads;
        t_success = MCAdGetAds(ctxt, t_ads);
        if (t_success && t_ads.Borrow() != nil)
            ep.copysvalue(t_ads.Borrow());
    }
    
    if (t_success)
        MCresult->store(ep, False);
#endif
    
    if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
    
}

//////////////////////////////////////////////////////////////////////////////////////

//iphonePickMedia [multiple] [, music, podCast, audioBook, anyAudio, movie, tv, videoPodcast, musicVideo, videoITunesU, anyVideo]
Exec_stat MCHandleIPhonePickMedia(void *context, MCParameter *p_parameters)
{
/*	bool t_success, t_allow_multipe_items;
	char *t_option_list;
    const char *r_return_media_types;
	MCMediaType t_media_types;
	
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
	
    MCExecPoint ep(nil, nil, nil);
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types += kMCsongs;
		else if (MCCStringEqualCaseless(t_option_list, "podCast"))
			t_media_types += kMCpodcasts;
		else if (MCCStringEqualCaseless(t_option_list, "audioBook"))
			t_media_types += kMCaudiobooks;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += kMCmovies;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += kMCtv;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += kMCvideopodcasts;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += kMCmusicvideos;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += kMCmovies;
		}
#endif
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MCMediaTypeFromCString("podcast, songs, audiobook");;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MCMediaTypeFromCString("movies, tv, videoPodcasts, musicVideos, videoITunesU");;
#endif
	}
    MCExecContext ctxt(ep);
    
	// Call MCIPhonePickMedia to process the media pick selection.
    MCDialogExecPickMedia(ctxt, &t_media_types, t_allow_multipe_items, r_return_media_types);
*/	
	return ES_NORMAL;
}

Exec_stat MCHandlePick(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_use_cancel, t_use_done, t_use_picker, t_use_checkmark, t_more_optional, t_success;
	t_success = true;
	t_more_optional = true;
	t_use_checkmark = false;
	t_use_done = false;
	t_use_cancel = false;
	t_use_picker = false;
	
    
    MCAutoArray<MCStringRef> t_option_lists;
    MCAutoArray<uindex_t> t_indices;
    
    MCStringRef t_string_param;
   	uint32_t t_initial_index;
    // get the mandatory options list and the initial index
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
	t_success = MCParseParameters(p_parameters, "x", t_string_param);
    if (t_success)
    {
        t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
        if (!t_success)
        {
            // Degrade gracefully, even if the second mandatory parameter is not supplied.
            t_initial_index = 0;
            t_success = true;
        }
        t_option_lists . Push(t_string_param);
        t_indices . Push(t_initial_index);
    }
    
    // get further options lists if they exist
    while (t_success && t_more_optional)
    {
    	t_success = MCParseParameters(p_parameters, "x", t_string_param);
        if (t_success)
        {
            if (t_string_param != nil)
            {
                if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "cancelDone", kMCCompareCaseless) ||
                    MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
                        t_more_optional = false;
                else
                {
                    t_success = MCParseParameters(p_parameters, "u", &t_initial_index);
                    if (!t_success)
                    {
                        // Degrade gracefully, even if the second mandatory parameter is not supplied.
                        t_initial_index = 0;
                        t_success = true;
                    }
                    t_option_lists . Push(t_string_param);
                    t_indices . Push(t_initial_index);
                }
            }
            else
                t_more_optional = false;
        }
    }
    
    // now process any additional parameters
    
    MCPickButtonType t_type = kMCPickButtonNone;
    
    while (t_success && t_string_param != nil)
    {
        if (MCStringIsEqualToCString(t_string_param, "checkmark", kMCCompareCaseless))
            t_use_checkmark = true;
        else if (MCStringIsEqualToCString(t_string_param, "cancel", kMCCompareCaseless))
            t_type = kMCPickButtonCancel;
        else if (MCStringIsEqualToCString(t_string_param, "done", kMCCompareCaseless))
            t_type = kMCPickButtonDone;
        else if (MCStringIsEqualToCString(t_string_param, "canceldone", kMCCompareCaseless))
            t_type = kMCPickButtonCancelAndDone;
        else if (MCStringIsEqualToCString(t_string_param, "picker", kMCCompareCaseless))
            t_use_picker = true;
        
        MCValueRelease(t_string_param);
        t_success = MCParseParameters(p_parameters, "x", t_string_param);
    }
    
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
	// call the Exec method to process the pick wheel
	MCPickExecPickOptionByIndex(ctxt, (int)kMCLines, t_option_lists . Ptr(), t_option_lists . Size(), t_indices . Ptr(), t_indices . Size(),t_use_checkmark, t_use_picker, t_use_cancel, t_use_done, MCtargetptr->getrect());
    
	if (t_success)
    {
        // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
        // set the value of the 'it' variable
		if (MCresult->isempty())
		{
			MCAutoStringRef t_value;
			/* UNCHECKED */ ep . copyasstringref(&t_value);
			ctxt . SetTheResultToValue(*t_value);
		}
    }

    // Free memory
    for (uindex_t i = 0; i < t_option_lists . Size(); i++)
        MCValueRelease(t_option_lists[i]);
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}
/*
// HC-2011-10-12 [[ Media Picker ]] Implementation of media picker functionality.
Exec_stat MCHandleIPhonePickMedia(void *context, MCParameter *p_parameters)
{
	bool t_success, t_allow_multipe_items;
	char *t_option_list;
	MPMediaType t_media_types;
	NSString *r_return_media_types;
    
	t_success = true;
	t_allow_multipe_items = false;
	t_media_types = 0;
	
	t_option_list = nil;
    
	// Get the options list.
	t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	while (t_success)
	{
		if (MCCStringEqualCaseless(t_option_list, "true"))
			t_allow_multipe_items = true;
		else if (MCCStringEqualCaseless(t_option_list, "music"))
			t_media_types += MPMediaTypeMusic;
		else if (MCCStringEqualCaseless(t_option_list, "podCast"))
			t_media_types += MPMediaTypePodcast;
		else if (MCCStringEqualCaseless(t_option_list, "audioBook"))
			t_media_types += MPMediaTypeAudioBook;
		else if (MCCStringEqualCaseless(t_option_list, "anyAudio"))
			t_media_types += MPMediaTypeAnyAudio;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
		{
			if (MCCStringEqualCaseless(t_option_list, "movie"))
				t_media_types += MPMediaTypeMovie;
			else if (MCCStringEqualCaseless(t_option_list, "tv"))
				t_media_types += MPMediaTypeTVShow;
			else if (MCCStringEqualCaseless(t_option_list, "videoPodcast"))
				t_media_types += MPMediaTypeVideoPodcast;
			else if (MCCStringEqualCaseless(t_option_list, "musicVideo"))
				t_media_types += MPMediaTypeMusicVideo;
			else if (MCCStringEqualCaseless(t_option_list, "videoITunesU"))
				t_media_types += MPMediaTypeVideoITunesU;
			else if (MCCStringEqualCaseless(t_option_list, "anyVideo"))
				t_media_types += MPMediaTypeAnyVideo;
		}
#endif
		t_success = MCParseParameters(p_parameters, "s", &t_option_list);
	}
	if (t_media_types == 0)
	{
		t_media_types = MPMediaTypeAnyAudio;
#ifdef __IPHONE_5_0
		if (MCmajorosversion >= 500)
			t_media_types += MPMediaTypeAnyVideo;
#endif
	}
	// Call MCIPhonePickMedia to process the media pick selection.
	t_success = MCIPhonePickMedia(t_allow_multipe_items, t_media_types, r_return_media_types);
	
	if (t_success && r_return_media_types != nil)
	{
		MCresult -> sets ([r_return_media_types cStringUsingEncoding:NSMacOSRomanStringEncoding]);
	}
	return ES_NORMAL;
}
*/
// MM-2012-11-02: Temporarily refactored mobilePickDate to use the old syntax (rather than three separate pick date, pick time, pick date and time).
Exec_stat MCHandlePickDate(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success;
	t_success = true;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    char *t_type;
    t_type = nil;
    
    if (t_success && p_parameters != nil)
  		t_success = MCParseParameters(p_parameters, "s", &t_type);
    
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success && p_parameters != nil)
    {
        char *t_button;
        t_button = nil;
		t_success = MCParseParameters(p_parameters, "s", &t_button);
        if (t_success)
        {
            if (MCCStringEqualCaseless("cancel", t_button))
                t_button_type = kMCPickButtonCancel;
            else if (MCCStringEqualCaseless("done", t_button))
                t_button_type = kMCPickButtonDone;
            else if (MCCStringEqualCaseless("canceldone", t_button))
                t_button_type = kMCPickButtonCancelAndDone;
        }
        MCCStringFree(t_button);
    }
    
    MCExecContext ctxt(ep);
     
	if (t_success)
    {
        // MM-2012-03-15: [[ Bug ]] Make sure we handle no type being passed.
        if (t_type == nil)
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("time", t_type))
            MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
        else if (MCCStringEqualCaseless("datetime", t_type))
            MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
        else
            MCPickExecPickDate(ctxt, *t_current, *t_start, *t_end, (intenum_t)t_button_type, MCtargetptr->getrect());
    }
    
    MCCStringFree(t_type);
    
    // at the moment, this is the only way to return a value from the function.  pick (date/time/...) should probably
    // set the value of the 'it' variable
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

Exec_stat MCHandlePickTime(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;           
    }

    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
    
	if (t_success)
		MCPickExecPickTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
    
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}


Exec_stat MCHandlePickDateAndTime(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    
	bool t_success, t_use_done, t_use_cancel;
	t_success = true;
	t_use_done = false;
	t_use_cancel = false;
	
    bool t_use_current = false;
    bool t_use_start = false;
    bool t_use_end = false;
	
    MCAutoStringRef t_current, t_start, t_end;
    
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_current);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_start);
        p_parameters = p_parameters->getnext();
    }
	
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        t_success = ep . copyasstringref(&t_end);
        p_parameters = p_parameters->getnext();
    }
	
    int32_t t_step;
    int32_t *t_step_ptr = nil;
    if (t_success && p_parameters != nil)
        if (MCParseParameters(p_parameters, "i", &t_step))
            t_step_ptr = &t_step;
    
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_cancel);
	
	if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_use_done);
    
    MCPickButtonType t_button_type = kMCPickButtonNone;
    if (t_success)
    {
        if (t_use_cancel)
        {
            if (t_use_done)
                t_button_type = kMCPickButtonCancelAndDone;
            else
                t_button_type = kMCPickButtonCancel;
        }
        else if (t_use_done)
            t_button_type = kMCPickButtonDone;
    }
    
    MCExecContext ctxt(ep);
    ctxt.SetTheResultToEmpty();
       
	if (t_success)
		MCPickExecPickDateAndTime(ctxt, *t_current, *t_start, *t_end, t_step_ptr, (intenum_t)t_button_type, MCtargetptr->getrect());
    
    if (MCresult->isempty())
	{
		MCAutoStringRef t_value;
		/* UNCHECKED */ ep . copyasstringref(&t_value);
        ctxt . SetTheResultToValue(*t_value);
	}
    
	if (!ctxt . HasError())
		return ES_NORMAL;
    
	return ES_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////