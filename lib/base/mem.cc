/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "mem.h"

#include <base/logging.h>

#include "os.h"
#include "utils.h"

#if defined(TCMALLOC)
#include <tcmalloc/malloc_extension.h>
#elif defined(GPERFTOOLS_TCMALLOC)
#include <gperftools/malloc_extension.h>
#endif

using namespace MSF;

namespace MSF {

void *BasicNumaAllocOnNode(size_t size, int node) {
  return numa_alloc_onnode(size, node);
}

void *BasicNumaAllocLocal(size_t size) { return numa_alloc_local(size); }

void *BasicNumaAllocInterleaved(size_t size) {
  return numa_alloc_interleaved(size);
}

void *BasicNumaAlloc(size_t size) { return numa_alloc(size); }

void BasicNumaFree(void *start, size_t size) { numa_free(start, size); }

int BasicNumaPeferred(void) { return numa_preferred(); }

void BasicNumaSetLocalAlloc(void) { numa_set_localalloc(); }

int BasicNumaNodeOfCpu(int cpu) { return numa_node_of_cpu(cpu); }

int BasicNumaRunOnNode(int node) { return numa_run_on_node(node); }

int BasicNumaNumTaskCpus(void) { return numa_num_task_cpus(); }

int BasicNumaNumTaskNodes(void) { return numa_num_task_nodes(); }

int BasicNumaNumConfNodes(void) { return numa_num_configured_nodes(); }

int BasicNumaNumConfCpus(void) { return numa_num_configured_cpus(); }

int BasicNumaNumPossibleNodes(void) { return numa_num_possible_nodes(); }

int BasicNumaNumPossibleCpus(void) { return numa_num_possible_cpus(); }

int BasicNumaPinNode(int cpu) {
  int node = BasicNumaNodeOfCpu(cpu);
  /* pin to node */
  int ret = BasicNumaRunOnNode(node);

  if (ret) return -1;

  /* is numa_run_on_node() guaranteed to take effect immediately? */
  sched_yield();

  return -1;
}

void *BaseAllocHugePages(size_t size) {
  size_t real_size;
  void *ptr = nullptr;

#ifndef WIN32
  LOG(ERROR) << "sysconf pagesize is invalid";

  long page_size = 0;  // g_os->pagesize;

  if (page_size < 0) {
    LOG(ERROR) << "sysconf pagesize is invalid";
    return nullptr;
  }

  real_size = MSF_ALIGN(size, page_size);
  ptr = BasicAlignedAlloc(page_size, real_size);
  if (ptr == nullptr) {
    LOG(ERROR) << "posix_memalign failed sz:" << real_size;
    return nullptr;
  }
  memset(ptr, 0, real_size);
  return ptr;
#else
  /* Use 1 extra page to store allocation metadata */
  /* (libhugetlbfs is more efficient in this regard) */
  real_size = MSF_ALIGN(size + MSF_HUGE_PAGE_SIZE, MSF_HUGE_PAGE_SIZE);

  ptr = BasicMemMap(real_size);
  if (!ptr || ptr == MAP_FAILED) {
    /* The mmap() call failed. Try to malloc instead */
    long page_size = 0;  // g_os->pagesize;

    if (page_size < 0) {
      LOG(ERROR) << "sysconf pagesize is invalid";
      return NULL;
    }
    LOG(INFO) << "huge pages allocation failed, allocating regular pages";

    real_size = MSF_ALIGN(size + MSF_HUGE_PAGE_SIZE, page_size);
    // WIN32
    ptr = _aligned_malloc(page_size, real_size);
    // _aligned_free(ptr);
    // ptr = BasicAlignedAlloc(page_size, real_size);
    if (ptr == nullptr) {
      LOG(ERROR) << "posix_memalign failed sz:" << real_size;
      return nullptr;
    }
    memset(ptr, 0, real_size);
    real_size = 0;
  } else {
    LOG(INFO) << "Allocated huge page sz: " << real_size << "successful.";
  }
  /* Save real_size since mmunmap() requires a size parameter */
  *((size_t *)ptr) = real_size;
  /* Skip the page with metadata */
  return (char *)ptr + MSF_HUGE_PAGE_SIZE;
#endif
}

void BasicFreeHugePages(void *ptr) {
  void *real_ptr;
  size_t real_size;

  if (!ptr) return;

#ifndef WIN32
  free(ptr);
  return;
#endif

  /* Jump back to the page with metadata */
  real_ptr = (char *)ptr - MSF_HUGE_PAGE_SIZE;
  /* Read the original allocation size */
  real_size = *((size_t *)real_ptr);
  if (real_size != 0) {
    /* The memory was allocated via mmap()
       and must be deallocated via munmap()
       */
    munmap(real_ptr, real_size);
  } else {
    /* The memory was allocated via malloc()
       and must be deallocated via free()
       */
    free(real_ptr);
  }
}

void *BaseNumaAllocPages(size_t bytes, int node) {
  // if (g_os->en_numa < 0) {
  //     return NULL;
  // }

  long page_size = 0;  // g_os->pagesize;
  size_t real_size = MSF_ALIGN((bytes + page_size), page_size);
  void *p = BasicNumaAllocOnNode(real_size, node);
  if (!p) {
    LOG(ERROR) << "numa_alloc_onnode failed sz: " << real_size;
    return NULL;
  }
  /* force the OS to allocate physical memory for the region */
  memset(p, 0, real_size);

  /* Save real_size since numa_free() requires a size parameter */
  *((size_t *)p) = real_size;

  /* Skip the page with metadata */
  return (char *)p + page_size;
}

void BaseNumaFreePages(void *ptr) {
  void *real_ptr;
  size_t real_size;

  if (!ptr) return;

  long page_size = 0;  // g_os->pagesize;

  /* Jump back to the page with metadata */
  real_ptr = (char *)ptr - page_size;
  /* Read the original allocation size */
  real_size = *((size_t *)real_ptr);

  if (real_size != 0)
    /* The memory was allocated via numa_alloc()
       and must be deallocated via numa_free()
       */
    BasicNumaFree(real_ptr, real_size);
}

/********************Interface NUMA**************************************/
int numa_node_to_cpusmask(int node, uint64_t *cpusmask, int *nr) {
  struct bitmask *mask;
  uint64_t bmask = 0;
  int retval = -1;
  uint32_t i;

  mask = numa_allocate_cpumask();
  retval =
      numa_node_to_cpus(node, (unsigned long *)mask, sizeof(struct bitmask));
  if (retval < 0) goto cleanup;

  *nr = 0;
  for (i = 0; i < mask->size && i < 64; i++) {
    if (numa_bitmask_isbitset(mask, i)) {
      MSF_SET_BIT(i, &bmask);
      (*nr)++;
    }
  }

  retval = 0;
cleanup:
  *cpusmask = bmask;

  numa_free_cpumask(mask);
  return retval;
}

void msf_numa_tonode_memory(void *mem, size_t size, int node) {
  numa_tonode_memory(mem, size, node);
}

long long msf_numa_node_size(int node, long *freep) {
  return numa_node_size(node, freep);
}

int msf_numa_distance(int node1, int node2) {
  return numa_distance(node1, node2);
}

int64_t RamTotal() {
#if defined(__APPLE__)
  int64_t memsize = 0;
  size_t size = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0)
    return memsize;

  return -1;
#elif defined(unix) || defined(__unix) || defined(__unix__)
  int64_t pages = ::sysconf(_SC_PHYS_PAGES);
  int64_t page_size = ::sysconf(_SC_PAGESIZE);
  if ((pages > 0) && (page_size > 0)) return pages * page_size;

  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullTotalPhys;
#else
#error Unsupported platform
#endif
}

int64_t RamFree() {
#if defined(__APPLE__)
  mach_port_t host_port = mach_host_self();
  if (host_port == MACH_PORT_NULL) return -1;

  vm_size_t page_size = 0;
  host_page_size(host_port, &page_size);

  vm_statistics_data_t vmstat;
  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
  kern_return_t kernReturn =
      host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vmstat, &count);
  if (kernReturn != KERN_SUCCESS) return -1;

  [[maybe_unused]] int64_t used_mem =
      (vmstat.active_count + vmstat.inactive_count + vmstat.wire_count) *
      page_size;
  int64_t free_mem = vmstat.free_count * page_size;
  return free_mem;
#elif defined(unix) || defined(__unix) || defined(__unix__)
  int64_t pages = ::sysconf(_SC_AVPHYS_PAGES);
  int64_t page_size = ::sysconf(_SC_PAGESIZE);
  if ((pages > 0) && (page_size > 0)) return pages * page_size;

  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullAvailPhys;
#else
#error Unsupported platform
#endif
}

#if defined(TCMALLOC)

uint64_t totalCurrentlyAllocated() {
  return tcmalloc::MallocExtension::GetNumericProperty(
      "generic.current_allocated_bytes").value_or(0);
}

uint64_t totalCurrentlyReserved() {
  // In Google's tcmalloc the semantics of generic.heap_size has
  // changed: it doesn't include unmapped bytes.
  return tcmalloc::MallocExtension::GetNumericProperty("generic.heap_size")
             .value_or(0) +
         tcmalloc::MallocExtension::GetNumericProperty(
             "tcmalloc.pageheap_unmapped_bytes").value_or(0);
}

uint64_t totalThreadCacheBytes() {
  return tcmalloc::MallocExtension::GetNumericProperty(
      "tcmalloc.current_total_thread_cache_bytes").value_or(0);
}

uint64_t totalPageHeapFree() {
  return tcmalloc::MallocExtension::GetNumericProperty(
      "tcmalloc.pageheap_free_bytes").value_or(0);
}

uint64_t totalPageHeapUnmapped() {
  return tcmalloc::MallocExtension::GetNumericProperty(
      "tcmalloc.pageheap_unmapped_bytes").value_or(0);
}

uint64_t totalPhysicalBytes() {
  return tcmalloc::MallocExtension::GetProperties()
      ["generic.physical_memory_used"].value;
}

void dumpStatsToLog() {
  LOG(INFO) << "TCMalloc stats: " << tcmalloc::MallocExtension::GetStats();
}

#elif defined(GPERFTOOLS_TCMALLOC)

uint64_t totalCurrentlyAllocated() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "generic.current_allocated_bytes", &value);
  return value;
}

uint64_t totalCurrentlyReserved() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty("generic.heap_size", &value);
  return value;
}

uint64_t totalThreadCacheBytes() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "tcmalloc.current_total_thread_cache_bytes", &value);
  return value;
}

uint64_t totalPageHeapFree() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "tcmalloc.pageheap_free_bytes", &value);
  return value;
}

uint64_t totalPageHeapUnmapped() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "tcmalloc.pageheap_unmapped_bytes", &value);
  return value;
}

uint64_t totalPhysicalBytes() {
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "generic.total_physical_bytes", &value);
  return value;
}

void dumpStatsToLog() {
  constexpr int buffer_size = 100000;
  auto buffer = std::make_unique<char[]>(buffer_size);
  MallocExtension::instance()->GetStats(buffer.get(), buffer_size);
  LOG(INFO) << "TCMalloc stats: " << buffer.get();
}

#endif

#if defined(TCMALLOC) || defined(GPERFTOOLS_TCMALLOC)
// TODO(zyfjeff): Make max unfreed memory byte configurable
constexpr uint64_t MAX_UNFREED_MEMORY_BYTE = 100 * 1024 * 1024;
#endif

void ReleaseFreeMemory() {
#if defined(TCMALLOC)
  tcmalloc::MallocExtension::ReleaseMemoryToSystem(MAX_UNFREED_MEMORY_BYTE);
#elif defined(GPERFTOOLS_TCMALLOC)
  MallocExtension::instance()->ReleaseFreeMemory();
#endif
}

/*
  The purpose of this function is to release the cache introduced by tcmalloc,
  mainly in xDS config updates, admin handler, and so on. all work on the main
  thread,
  so the overall impact on performance is small.
  Ref: https://github.com/envoyproxy/envoy/pull/9471#discussion_r363825985
*/
void TryShrinkHeap() {
#if defined(TCMALLOC) || defined(GPERFTOOLS_TCMALLOC)
  auto total_physical_bytes = totalPhysicalBytes();
  auto allocated_size_by_app = totalCurrentlyAllocated();

  assert(total_physical_bytes >= allocated_size_by_app);

  if ((total_physical_bytes - allocated_size_by_app) >=
      MAX_UNFREED_MEMORY_BYTE) {
    ReleaseFreeMemory();
  }
#endif
}

}  // namespace MSF
