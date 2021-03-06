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

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "execpt.h"
#include "scriptpt.h"
#include "visual.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"

MCEffectList::~MCEffectList()
{
	while(arguments != NULL)
	{
		MCEffectArgument *t_kv = arguments;
		arguments = t_kv -> next;
		delete[] t_kv -> key;
		delete[] t_kv -> value;
		delete t_kv;
	}
	
	delete name;
	delete sound;
}

MCVisualEffect::~MCVisualEffect()
{
	while(parameters != NULL)
	{
		KeyValue *t_kv = parameters;
		parameters = t_kv -> next;
		delete[] t_kv -> key;
		delete t_kv -> value;
		delete t_kv;
	}
	delete nameexp;
	delete soundexp;
}

Parse_stat MCVisualEffect::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_VISUAL, TT_VISUAL, VE_EFFECT);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_VISUAL_NOTVISUAL, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_VISUAL, te) == PS_NORMAL && te->which != VE_EFFECT)
		effect = (Visual_effects)te->which;
	sp.backup();
	if (sp.parseexp(False, True, &nameexp) != PS_NORMAL)
	{
		MCperror->add(PE_VISUAL_NOTVISUAL, sp);
		return PS_ERROR;
	}
	
	while (True)
	{
		// MW-2005-05-08: [[CoreImage]] We've reached the end of the command so return and
		//   skip parsing of extended parameters.
		if (sp.next(type) != PS_NORMAL)
			return PS_NORMAL;
			
		if (type != ST_ID && type != ST_LIT)
		{
			sp.backup();
			// MW-2005-05-08: [[CoreImage]] We've reached the end of the command so return and
			//   skip parsing of extended parameters.
			return PS_NORMAL;
		}
		
		// MW-2005-05-08: [[CoreImage]] If the keyword is WITH break-out and continue processing
		//   else it's an error.
		if (sp.lookup(SP_VISUAL, te) != PS_NORMAL)
		{
			MCperror -> add(PE_VISUAL_NOTVISUAL, sp);
			return PS_ERROR;
		}
			
		if (te -> which == VE_WITH)
			break;
			
		switch (te->which)
		{
		case VE_EXTRA:
		case VE_TO:
		case VE_FROM:
			break;
		case VE_CLOSE:
		case VE_OPEN:
		case VE_IN:
		case VE_OUT:
		case VE_DOWN:
		case VE_LEFT:
		case VE_RIGHT:
		case VE_UP:
		case VE_BOTTOM:
		case VE_CENTER:
		case VE_TOP:
			if (direction != VE_UNDEFINED)
			{
				MCperror->add(PE_VISUAL_DUPDIRECTION, sp);
				return PS_ERROR;
			}
			direction = (Visual_effects)te->which;
			break;
		case VE_VERY:
		case VE_FAST:
		case VE_SLOW:
		case VE_NORMAL:
			if (speed == VE_VERY)
				if (te->which == VE_FAST)
					speed = VE_VFAST;
				else
					speed = VE_VSLOW;
			else
				speed = (Visual_effects)te->which;
			break;
		case VE_BLACK:
		case VE_CARD:
		case VE_GRAY:
		case VE_INVERSE:
		case VE_WHITE:
			image = (Visual_effects)te->which;
			break;
		}
	}
	
	// MW-2005-05-08: [[CoreImage]] We find ourselves here so we expect a sound parameter
	//   potentially followed by a list of parameters
	if (sp . skip_token(SP_VISUAL, TT_VISUAL, VE_SOUND) == PS_NORMAL)
	{
		if (sp . parseexp(True, False, &soundexp) != PS_NORMAL)
		{
			MCperror -> add(PE_VISUAL_NOTVISUAL, sp);
			return PS_ERROR;
		}
		
		if (sp . skip_token(SP_FACTOR, TT_BINOP, O_AND) != PS_NORMAL)
			return PS_NORMAL;
	}
		
	do
	{
		char *t_key = NULL;
		MCExpression *t_value = NULL;
		bool t_has_id = false;
	
		if (sp . next(type) == PS_NORMAL && type == ST_ID)
			t_key = sp . gettoken() . clone();
		
		if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_ID) == PS_NORMAL)
			t_has_id = true;
		
		if (t_key != NULL && sp . parseexp(True, False, &t_value) == PS_NORMAL)
		{
			KeyValue *t_kv;
			t_kv = new KeyValue;
			t_kv -> next = parameters;
			t_kv -> key = t_key;
			t_kv -> value = t_value;
			t_kv -> has_id = t_has_id;
			parameters = t_kv;
		}
		else
		{
			delete[] t_key;
			MCperror -> add(PE_VISUAL_BADPARAM, sp);
		}
	}
	while(sp . skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL);
	
	return PS_NORMAL;
}

Exec_stat MCVisualEffect::exec(MCExecPoint &ep)
{
	MCEffectList *effectptr = MCcur_effects;

	if (nameexp -> eval(ep) != ES_NORMAL)
	{
		MCeerror -> add(EE_VISUAL_BADEXP, line, pos);
		return ES_ERROR;
	}
	char *ename = ep.getsvalue().clone();
	MCScriptPoint spt(ep);
	MCerrorlock++;

	// reset values so expression parsing can proceed
	MCExpression *oldnameexp = nameexp;
	nameexp = NULL;
	Visual_effects olddirection = direction;
	direction = VE_UNDEFINED;
	Visual_effects oldspeed = speed;
	speed = VE_NORMAL;

	parse(spt); // get values from variable, don't worry about errors
	
	// restore values if not set in expression
	delete nameexp;
	nameexp = oldnameexp;
	if (direction == VE_UNDEFINED)
		direction = olddirection;
	if (speed == VE_NORMAL)
		speed = oldspeed;
	
	MCerrorlock--;
	
	char *sname = NULL;
	if (soundexp != NULL)
	{
		if (soundexp->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_VISUAL_BADEXP, line, pos);
			delete ename;
			return ES_ERROR;
		}
		sname = ep.getsvalue().clone();
	}
	
	MCEffectArgument *t_arguments = NULL;
	for(KeyValue *t_parameter = parameters; t_parameter != NULL; t_parameter = t_parameter -> next)
	{
		if (t_parameter -> value -> eval(ep) != ES_NORMAL)
		{
			while(t_arguments != NULL)
			{
				MCEffectArgument *t_kv = t_arguments;
				t_arguments = t_kv -> next;
				delete[] t_kv -> key;
				delete[] t_kv -> value;
				delete t_kv;
			}
			delete[] sname;
			delete[] ename;
			
			MCeerror -> add(EE_VISUAL_BADEXP, line, pos);
			return ES_ERROR;
		}
	
		if (t_parameter -> has_id)
			ep . insert("id ", 0, 0);
	
		MCEffectArgument *t_kv;
		t_kv = new MCEffectArgument;
		t_kv -> next = t_arguments;
		t_kv -> key = strdup(t_parameter -> key);
		t_kv -> value = ep . getsvalue() . clone();
		t_arguments = t_kv;
	}
	
	if (effectptr == NULL)
		MCcur_effects = effectptr = new MCEffectList;
	else
	{
		while (effectptr->next != NULL)
			effectptr = effectptr->next;
		effectptr->next = new MCEffectList;
		effectptr = effectptr->next;
	}
	
	effectptr -> type = effect;
	effectptr -> direction = direction;
	effectptr -> speed = speed;
	effectptr -> image = image;
	effectptr -> name = ename;
	effectptr -> sound = sname;
	effectptr -> arguments = t_arguments;
	
	return ES_NORMAL;
}
