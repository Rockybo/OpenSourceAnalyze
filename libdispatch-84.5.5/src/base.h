/*
 * Copyright (c) 2008-2009 Apple Inc. All rights reserved.
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

#ifndef __DISPATCH_BASE__
#define __DISPATCH_BASE__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#endif

#ifdef __cplusplus
/*
 * Dispatch objects are NOT C++ objects. Nevertheless, we can at least keep C++
 * aware of type compatibility.
 */
typedef struct dispatch_object_s {
private:
	dispatch_object_s();
	~dispatch_object_s();
	dispatch_object_s(const dispatch_object_s &);
	void operator=(const dispatch_object_s &);
} *dispatch_object_t;
#else
typedef union {
	struct dispatch_object_s *_do; // dispatch_object_s结构体，这个是 GCD 的基类
	struct dispatch_continuation_s *_dc; // 任务类型，通常 dispatch_async内的block最终都会封装成这个数据类型
	struct dispatch_queue_s *_dq; // 任务队列，我们创建的对列都是这个类型的，不管是串行队列还是并发队列
    struct dispatch_queue_attr_s *_dqa; // 任务队列的属性，任务队列的属性里面包含了任务队列里面的一些操作函数，可以表明这个任务队列是串行还是并发队列
	struct dispatch_group_s *_dg; // GCD的group
	struct dispatch_source_s *_ds; // GCD的sourece ，可以监测内核事件，文件读写事件和 socket 通信事件等
	struct dispatch_source_attr_s *_dsa; // sourece的属性。
	struct dispatch_semaphore_s *_dsema; // 信号量，如果了解过 pthread 都知道，信号量可以用来调度线程
} dispatch_object_t __attribute__((transparent_union));
// 注意__attribute__((transparent_union))是一种典型的 透明联合体，
/**
    透明联合类型削弱了C语言的类型检测机制。起到了类似强制类型转换的效果，通过透明联合体，使得dispatch_object_s就像C++中的基类，在函数参数类型的转换上，给C语言的参数传递带来极大的方便。
 考虑到在底层，类型实质上是不存在的，因此所谓的透明联合类型，也就是在一定程度上打破了类型对我们的束缚，使数据以一种更底层的角度呈现在我们面前。不过这样也弱化了C语言对类型的检测，由此也可能带来一些很严重的错误。
 */
#endif

typedef void (*dispatch_function_t)(void *);

#ifdef __cplusplus
#define DISPATCH_DECL(name) typedef struct name##_s : public dispatch_object_s {} *name##_t;
#else
/*! @parseOnly */
#define DISPATCH_DECL(name) typedef struct name##_s *name##_t;
#endif

#ifdef __GNUC__
#define DISPATCH_NORETURN __attribute__((__noreturn__))
#define DISPATCH_NOTHROW __attribute__((__nothrow__))
#define DISPATCH_NONNULL1 __attribute__((__nonnull__(1)))
#define DISPATCH_NONNULL2 __attribute__((__nonnull__(2)))
#define DISPATCH_NONNULL3 __attribute__((__nonnull__(3)))
#define DISPATCH_NONNULL4 __attribute__((__nonnull__(4)))
#define DISPATCH_NONNULL5 __attribute__((__nonnull__(5)))
#define DISPATCH_NONNULL6 __attribute__((__nonnull__(6)))
#define DISPATCH_NONNULL7 __attribute__((__nonnull__(7)))
#if __clang__
// rdar://problem/6857843
#define DISPATCH_NONNULL_ALL
#else
#define DISPATCH_NONNULL_ALL __attribute__((__nonnull__))
#endif
#define DISPATCH_SENTINEL __attribute__((__sentinel__))
#define DISPATCH_PURE __attribute__((__pure__))
#define DISPATCH_WARN_RESULT __attribute__((__warn_unused_result__))
#define DISPATCH_MALLOC __attribute__((__malloc__))
#else
/*! @parseOnly */
#define DISPATCH_NORETURN
/*! @parseOnly */
#define DISPATCH_NOTHROW
/*! @parseOnly */
#define DISPATCH_NONNULL1
/*! @parseOnly */
#define DISPATCH_NONNULL2
/*! @parseOnly */
#define DISPATCH_NONNULL3
/*! @parseOnly */
#define DISPATCH_NONNULL4
/*! @parseOnly */
#define DISPATCH_NONNULL5
/*! @parseOnly */
#define DISPATCH_NONNULL6
/*! @parseOnly */
#define DISPATCH_NONNULL7
/*! @parseOnly */
#define DISPATCH_NONNULL_ALL
/*! @parseOnly */
#define DISPATCH_SENTINEL
/*! @parseOnly */
#define DISPATCH_PURE
/*! @parseOnly */
#define DISPATCH_WARN_RESULT
/*! @parseOnly */
#define DISPATCH_MALLOC
#endif

#endif
