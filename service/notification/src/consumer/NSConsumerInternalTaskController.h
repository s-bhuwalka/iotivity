//******************************************************************
//
// Copyright 2016 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef _NS_CONSUMER_INTERNAL_TASK_CONTROLLER_H_
#define _NS_CONSUMER_INTERNAL_TASK_CONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "NSStructs.h"
#include "NSConsumerMemoryCache.h"
#include "NSConsumerCommunication.h"

NSCacheList ** NSGetMessageCacheList(void);

void NSSetMessageCacheList(NSCacheList *);

NSCacheList ** NSGetProviderCacheList(void);

void NSCancelAllSubscription(void);

void NSSetProviderCacheList(NSCacheList *);

void NSDestroyMessageCacheList(void);

void NSDestroyInternalCachedList(void);

NSMessage * NSMessageCacheFind(const char *);

NSProvider_internal * NSProviderCacheFind(const char *);

NSProvider_internal * NSFindProviderFromAddr(OCDevAddr * addr);

void NSConsumerInternalTaskProcessing(NSTask *);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _NS_CONSUMER_INTERNAL_TASK_CONTROLLER_H_
