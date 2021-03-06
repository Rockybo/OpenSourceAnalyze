/*
 * Copyright (c) 2008-2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

#include "internal.h"

#undef dispatch_once
#undef dispatch_once_f


struct _dispatch_once_waiter_s {
	volatile struct _dispatch_once_waiter_s *volatile dow_next; //链表下一个节点
	_dispatch_thread_semaphore_t dow_sema; // 信号量
};

#define DISPATCH_ONCE_DONE ((struct _dispatch_once_waiter_s *)~0l)

#ifdef __BLOCKS__
void
dispatch_once(dispatch_once_t *val, dispatch_block_t block)
{
	struct Block_basic *bb = (void *)block;

	dispatch_once_f(val, block, (void *)bb->Block_invoke);
}
#endif

DISPATCH_NOINLINE
void
dispatch_once_f(dispatch_once_t *val, void *ctxt, dispatch_function_t func)
{
	// volatileg关键字编辑的变量vval，告诉编译器此指针指向的值随时可能被其他线程改变，从而使得编译器不对此指针进行代码编译优化。
	// 指针的最大的作用就是间接的改变变量的值
	struct _dispatch_once_waiter_s * volatile *vval =
			(struct _dispatch_once_waiter_s**)val;
	// 初始化一个结构体
	struct _dispatch_once_waiter_s dow = { NULL, 0 };
	
	// 声明辅助变量
	struct _dispatch_once_waiter_s *tail, *tmp;
	
	// 声明信号变量
	// uintptr_t sema
	_dispatch_thread_semaphore_t sema;

	
	// 内置函数 原子比较交换函数 __sync_bool_compare_and_swap
	// 判断vval与NULL是否相等，如果相等就返回YES，并将&dow的值赋给vval
	// 当dispatch_once第一次执行时，predicate也即val为0,地址并不为NULL，但是将0转成链表的时候vval为NULL，那么此“原子比较交换函数”将返回YES并将vval指向值赋值为&dow，即为“等待中”，_dispatch_client_callout其内部做了一些判定，但实际上是调用了func而已。到此，block中的用户代码执行完毕。
	// #1
	if (dispatch_atomic_cmpxchg(vval, NULL, &dow)) {
		
		//这是一个空的宏函数，大概是注释的作用吧
		dispatch_atomic_acquire_barrier();
		
		// 其实质是执行block
		_dispatch_client_callout(ctxt, func);

		// The next barrier must be long and strong.
		//
		// The scenario: SMP systems with weakly ordered memory models
		// and aggressive out-of-order instruction execution.
		//
		// The problem:
		//
		// The dispatch_once*() wrapper macro causes the callee's
		// instruction stream to look like this (pseudo-RISC):
		//
		//      load r5, pred-addr
		//      cmpi r5, -1
		//      beq  1f
		//      call dispatch_once*()
		//      1f:
		//      load r6, data-addr
		//
		// May be re-ordered like so:
		//
		//      load r6, data-addr
		//      load r5, pred-addr
		//      cmpi r5, -1
		//      beq  1f
		//      call dispatch_once*()
		//      1f:
		//
		// Normally, a barrier on the read side is used to workaround
		// the weakly ordered memory model. But barriers are expensive
		// and we only need to synchronize once! After func(ctxt)
		// completes, the predicate will be marked as "done" and the
		// branch predictor will correctly skip the call to
		// dispatch_once*().
		//
		// A far faster alternative solution: Defeat the speculative
		// read-ahead of peer CPUs.
		//
		// Modern architectures will throw away speculative results
		// once a branch mis-prediction occurs. Therefore, if we can
		// ensure that the predicate is not marked as being complete
		// until long after the last store by func(ctxt), then we have
		// defeated the read-ahead of peer CPUs.
		//
		// In other words, the last "store" by func(ctxt) must complete
		// and then N cycles must elapse before ~0l is stored to *val.
		// The value of N is whatever is sufficient to defeat the
		// read-ahead mechanism of peer CPUs.
		//
		// On some CPUs, the most fully synchronizing instruction might
		// need to be issued.

		// cpuid指令等待，使得其他线程的【读取到未初始化值的】预执行能被判定为猜测未命中，从而使得这些线程能够进入dispatch_once_f里的另一个分支从而进行等待
		dispatch_atomic_maximally_synchronizing_barrier();
		
		//这是一个空的宏函数，大概是注释的作用吧
		//dispatch_atomic_release_barrier(); // assumed contained in above
		
		// dispatch_atomic_xchg 其将第二个参数的值赋给第一个参数（解引用指针），然后返回第一个参数被赋值前的解引用值：
		// vval = &dow;
		// old = vval;
		// vval = DISPATCH_ONCE_DONE;// 置block完成标记，是置成NULL吗
		// tmp = old;
		tmp = dispatch_atomic_xchg(vval, DISPATCH_ONCE_DONE);
		tail = &dow;
		
		// tmp = 旧的vval = dow
		// vval = dow;
		// 接下来是对信号量链的处理：
		// 1.在block执行过程中，没有其他线程进入本函数来等待，则vval指向值保持为&dow，即tmp被赋值为&dow，即下方while循环不会被执行，此分支结束。
		// 2.在block执行过程中，有其他线程进入本函数来等待进入另一个分支，那么会构造一个信号量链表（vval指向值变为信号量链的头部，链表的尾部为&dow），此时就会当前分支进入while循环，在此while循环中，遍历链表，逐个signal每个信号量，然后结束循环。
		while (tail != tmp) {
			while (!tmp->dow_next) {
				// 此句是为了提示cpu减少额外处理，提升性能，节省电力。
				_dispatch_hardware_pause();
			}
			sema = tmp->dow_sema;
			tmp = (struct _dispatch_once_waiter_s*)tmp->dow_next;
			_dispatch_thread_semaphore_signal(sema);
		}
	} else {
		// #2
		// 当执行block分支#1未完成，且有线程再进入本函数时，将进入线程等待分支：
		// 先调用_dispatch_get_thread_semaphore创建一个信号量，此信号量被赋值给dow.dow_sema。
		// 然后进入一个无限for循环，假如发现vval的指向值已经为DISPATCH_ONCE_DONE，即“完成”，则直接break，然后调用_dispatch_put_thread_semaphore函数销毁信号量并退出函数
		// _dispatch_get_thread_semaphore内部使用的是“有即取用，无即创建”策略来获取信号量。
		dow.dow_sema = _dispatch_get_thread_semaphore();
		
		// 然后进入一个无限for循环
		for (;;) {
			tmp = *vval;
			
			// 假如发现vval的指向值已经为DISPATCH_ONCE_DONE，即“完成”，则直接break
			// 然后调用_dispatch_put_thread_semaphore函数销毁信号量并退出函数。
			if (tmp == DISPATCH_ONCE_DONE) {
				break;
			}
			dispatch_atomic_store_barrier();
			
			/*
			 假如vval的解引用值并非DISPATCH_ONCE_DONE，则进行一个“原子比较并交换”操作（此操作可以避免两个等待线程同时操作链表带来的问题）
			 假如此时vval指向值已不再是tmp（这种情况发生在多个线程同时进入线程等待分支#2，并交错修改链表）则for循环重新开始，再尝试重新获取一次vval来进行同样的操作；若指向值还是tmp，则将vval的指向值赋值为&dow，此时val->dow_next值为NULL，可能会使得block执行分支#1进行while等待（如前述），紧接着执行dow.dow_next = tmp这句来增加链表节点（同时也使得block执行分支#1的while等待结束），然后等待在信号量上，当block执行分支#1完成并遍历链表来signal时，唤醒、释放信号量，然后一切就完成了。
			 */
			// 此操作可以避免两个等待线程同时操作链表带来的问题
			// 判断vval与tmp是否相等，如果相等就返回YES，并将&dow的值赋给vval
			if (dispatch_atomic_cmpxchg(vval, tmp, &dow)) {
				dow.dow_next = tmp;
				
				// _dispatch_put_thread_semaphore内部使用的是“销毁旧的，存储新的”策略来缓存信号量
				_dispatch_thread_semaphore_wait(dow.dow_sema);
			}
		}
		_dispatch_put_thread_semaphore(dow.dow_sema);
	}
}
