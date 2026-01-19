#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <clean.h>
#include <colour.h>
#include <daemons.h>
#include <gmcp_defines.h>
#include <input.h>
#include <inventory.h>
#include <lib.h>
#include <messaging.h>
#include <modules.h>
#include <move.h>
#include <mudlib.h>
#include <objects.h>
#include <rooms.h>
#include <runtime_config.h>
#include <signal.h>
#include <type.h>

#define SIMUL_OB     "/adm/obj/simul_efun"
#define LOGIN_OB     "/adm/obj/login"

#define true 1
#define false 0
#define null ([])[0]
#define nullzilla ([])[0]

#define DATE "%F"
#define TIME "%T"
#define DATE_TIME "%F %T"

#endif // __GLOBAL_H__
