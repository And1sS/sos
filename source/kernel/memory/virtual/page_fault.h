#ifndef SOS_PAGE_FAULT_H
#define SOS_PAGE_FAULT_H

struct cpu_context;

struct cpu_context* handle_page_fault(struct cpu_context* context);

#endif // SOS_PAGE_FAULT_H
