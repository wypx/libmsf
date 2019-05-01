/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf_svc.h>
#include <msf_log.h>
#include <msf_file.h>

#define MSF_MOD_SVC "SVC"
#define MSF_SVC_LOG(level, ...) \
    log_write(level, MSF_MOD_SVC, MSF_FUNC_FILE_LINE, __VA_ARGS__)

/*
这里有一个函数名字是 foo.当我们同时使用test1.o和test2.o,
如果引用符号foo,最终使用到的是 func.c中定义的函数,
而不是 __foo, 虽然它有一个别名 foo.也就是说, 
我们最终使用到的函数会是"实际做事"的那个函数
当然, 单独使用 dummy.o 链接的话使用的是那个“不做事”的函数.
如果test2.o 中的 foo 不是 weak symbol 的话,在链接时会产生冲突,这就是我们要使用 weak 的原因。

glibc 的实现里面经常用 weak alias,比如它的 socket 函数，
在 C 文件里面你会看到一个 __socket 函数，它几乎什么都没有做,只是设置了一些错误代码,返回些东西而已.
在同一个 C 文件里面会再声明一个 __socket 的 weak alias 别名 socket.
实际完成工作的代码通过汇编来实现，在另外的汇编文件里面会有设置系统调用号,
执行 sysenter 或者 int 等动作去请求系统调用.
以前看 glibc 里面系统调用的实现的时候郁闷过很久,就是那个时候才知道了 weak alias 这个东西。

看来weak修饰符的原本含义是让weak弱类型的函数可以被其它同名函数覆盖（即不会发生冲突），
如果没有其它同名函数，就使用该weak函数，类似于是默认函数；
但是，受到“库”这一特别存在的影响，weak弱类型对库变得“不太可靠”了，
而且根据Weak definitions aren’t so weak来看，对于新版本的glibc，weak修饰符仅针对静态库产生影响。
weakref：可以说是weak与alias的结合版本（见上面英文引用），但在当前，被weakref修饰的符号必须是static的。
http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Function-Attributes.html 
Weak Alias 是 gcc 扩展里的东西，实际上是函数的属性
这个东西在库的实现里面可能会经常用到，比如 glibc 里面就用了不少。
抄录一段 gcc 手册里面的话解释下函数属性是干啥的
In GNU C, you declare certain things about functions called in your program 
which help the compiler optimize function calls and check your code more carefully.
*/

int __foo() {
    printf("I do no thing1.\n");
    return 0;
}
int foo() __attribute__ ((weak, alias("__foo")));
//等效  asm(".weak foo\n\t .set symbol1, __foo\n\t");

/*
    weak 和 alias 分别是两个属性。
    weak 使得 foo 这个符号在目标文件中作为 weak symbol 而不是 global symbol。
    用 nm 命令查看编译 dummy.c 生成的目标文件可用看到 foo 是一个 weak symbol，
    它前面的标记是 W。给函数加上weak属性时,即使函数没定义，函数被调用也可以编译成功。
    00000000 T __foo 00000000 W foo U puts

    而 alias 则使 foo 是 __foo 的一个别名,__foo 和 foo 必须在同一个编译单元中定义,否则会编译出错。

    /总结weak属性
    //（1）asm(".weak symbol1\n\t .set symbol1, symbol222\n\t");与
    //     void symbol1() __attribute__ ((weak,alias("symbol222")));等效。
    //（2）给函数加上weak属性时，即使函数没定义，函数被调用也可以编译成功。
    //（3）当有两个函数同名时，则使用强符号（也叫全局符号,即没有加weak的函数）来代替弱符号（加weak的函数）。
    //（4）当函数没有定义，但如果是“某个函数”的别名时，如果该函数被调用，就间接调用“某个函数”。

    */

s32 msf_svc_stub(const s8 *func, ...) {
    MSF_SVC_LOG(DBG_INFO, "Stub func[%s] return 401.", func);
    return 401;
}

void* msf_load_dynamic(const s8 *path) {
    void *handle = NULL;

    handle = MSF_DLOPEN_L(path);
    if (!handle) {
        MSF_SVC_LOG(DBG_ERROR, "DLOPEN_L() %s.", MSF_DLERROR());
    }

    return handle;
}

void *msf_load_symbol(void *handler, const s8 *symbol) {
    void *s = NULL;

    MSF_DLERROR();/* Clear any existing error */

    /* Writing: cosine = (double (*)(double)) dlsym(handle, "cos");
       would seem more natural, but the C99 standard leaves
       casting from "void *" to a function pointer undefined.
       The assignment used below is the POSIX.1-2003 (Technical
       Corrigendum 1) workaround; see the Rationale for the
       POSIX specification of dlsym(). */
       
    s = MSF_DLSYM(handler, symbol);

    if (!s) {
        MSF_SVC_LOG(DBG_ERROR, "DLSYM() %s.", MSF_DLERROR());
    }
    return s;
}

s32 msf_svcinst_init(struct svcinst *svc) {

    s32 rc;
    struct msf_file file;

    rc = msf_get_file_info(svc->svc_lib, &file, MSF_FILE_READ);
    if (rc == -1 || file.is_exist == 0 || file.is_dir == 0) {
         MSF_SVC_LOG(DBG_ERROR, "Svc chk %s failed, is_exist(%u) is_dir(%u).", 
            svc->svc_lib, file.is_exist, file.is_dir);
        //return -1;
    }

    svc->svc_handle = msf_load_dynamic(svc->svc_lib);
    if (!svc->svc_handle) {
        MSF_SVC_LOG(DBG_ERROR, "Svc load lib: '%s' failed.", svc->svc_lib);
        return -1;
    }

    svc->svc_cb = msf_load_symbol(svc->svc_handle, svc->svc_name);
    if (!svc->svc_cb) {
        MSF_SVC_LOG(DBG_ERROR, "Svc load symbol: '%s' failed.", svc->svc_name);
        MSF_DLCLOSE(svc->svc_handle);
        return -1;
    } else {
        MSF_SVC_LOG(DBG_ERROR, "Svc load: '%s' successful.", svc->svc_lib);
    }

    return 0;
}

