/* Copyright 2016 Pedro Falcato

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/* My Operating System is aiming for POSIX compliance, so this header is needed */
#ifndef _UNISTD_H
#define _UNISTD_H
/* This is complying with POSIX-1.2008 */
#define _POSIX_VERSION 	200809L
#define _POSIX2_VERSION 200809L

#define _XOPEN_VERSION 700

/* According to POSIX-1.2008, stuff that isn't supported shall be set with the value -1
 */
#define _POSIX_ADVISORY_INFO 	-1
#define _POSIX_ASYNCHRONOUS_IO 	-1
#define _POSIX_BARRIERS		-1
#define _POSIX_CHOWN_RESTRICTED	-1
#define _POSIX_CLOCK_SELECTION	-1
#define _POSIX_CPUTIME		200809L
#define _POSIX_FSYNC		-1
#define _POSIX_IPV6		-1
#define _POSIX_JOB_CONTROL	1
#define _POSIX_MAPPED_FILES	200809L
#define _POSIX_MEMLOCK		-1
#define _POSIX_MEMLOCK_RANGE	-1
#define _POSIX_MEMORY_PROTECTION 200809L
#define _POSIX_MESSAGE_PASSING	-1
#define _POSIX_MONOTONIC_CLOCK	-1
#define	_POSIX_NO_TRUNC		1
#define _POSIX_PRIORITIZED_IO	-1
#define _POSIX_PRIORITY_SCHEDULING -1
#define _POSIX_RAW_SOCKETS	-1
#define _POSIX_READER_WRITER_LOCKS 200809L
#define _POSIX_REALTIME_SIGNALS	200809L
#define _POSIX_REGEXP		200809L
#define _POSIX_SAVED_IDS	1
#define _POSIX_SEMAPHORES	200809L
#define _POSIX_SHARED_MEMORY_OBJECTS -1
#define _POSIX_SHELL		1
#define _POSIX_SPAWN		-1
#define _POSIX_SPIN_LOCKS	200809L
#define _POSIX_SPORADIC_SERVER	-1
#define _POSIX_SYNCHRONIZED_IO	-1
#define _POSIX_THREAD_ATTR_STACKADDR 200809L
#define _POSIX_THREAD_ATTR_STACKSIZE 200809L
#define _POSIX_THREAD_CPUTIME	200809L
#define _POSIX_THREAD_PRIO_INHERIT -1
#define _POSIX_THREAD_PRIO_PROTECT -1
#define _POSIX_THREAD_PRIORITY_SCHEDULING -1
#define _POSIX_THREAD_PROCESS_SHARED -1
#define _POSIX_THREAD_ROBUST_PRIO_INHERIT -1
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT -1
#define _POSIX_THREAD_SAFE_FUNCTIONS 200809L
#define _POSIX_THREAD_SPORADIC_SERVER -1
#define _POSIX_THREADS 		200809L
#define _POSIX_TIMEOUTS		200809L
#define _POSIX_TIMERS		200809L
#define _POSIX_TRACE		-1
#define _POSIX_TRACE_EVENT_FILTER -1
#define _POSIX_TRACE_LOG	-1
#define _POSIX_TYPED_MEMORY_OBJECTS -1
#ifdef i386
#define _POSIX_V6_ILP32_OFF32 	1
#endif // i386
#define _POSIX2_C_BIND 		200809L
#define _POSIX2_C_DEV		200809L
#define _POSIX2_CHAR_TERM	-1
#define _POSIX2_FORT_DEV	-1
#define _POSIX2_FORT_RUN	-1
#define _POSIX2_LOCALEDEF	-1
#define _POSIX2_PBS		-1
#define _POSIX2_PBS_ACCOUNTING	-1
#define _POSIX2_PBS_CHECKPOINT	-1
#define _POSIX2_PBS_LOCATE	-1
#define _POSIX2_PBS_MESSAGE	-1
#define _POSIX2_PBS_TRACK	-1
#define _POSIX2_SW_DEV		200809L
#define _POSIX2_UPE		200809L
#define _XOPEN_ENH_I18N		-1
#define _XOPEN_SHM		1
#define _XOPEN_UUCP		-1


#endif // _UNISTD_H