# Overview

rr is a lightweight tool for recording, replaying and debugging execution of applications (trees of processes and threads).
Debugging extends gdb with very efficient reverse-execution, which in combination with standard gdb/x86 features like hardware data watchpoints, makes debugging much more fun. More information about the project, including instructions on how to install, run, and build rr, is at [https://rr-project.org](https://rr-project.org). The best technical overview is currently the paper [Engineering Record And Replay For Deployability: Extended Technical Report](https://arxiv.org/pdf/1705.05937.pdf).

Or go directly to the [installation and building instructions](https://github.com/mozilla/rr/wiki/Building-And-Installing).

Please contribute!  Make sure to review the [pull request checklist](/CONTRIBUTING.md) before submitting a pull request.

If you find rr useful, please [add a testimonial](https://github.com/mozilla/rr/wiki/Testimonials).

# System requirements

* Linux kernel ≥ 3.11 is required (for `PTRACE_SETSIGMASK`).
* rr currently requires an Intel CPU with [Nehalem](https://en.wikipedia.org/wiki/Nehalem_%28microarchitecture%29) (2010) or later microarchitecture.
* Running in a VM guest is supported, as long as the VM supports virtualization of hardware performance counters. (VMware and KVM are known to work; Xen does not.)


# Dependencies that need to be included for Muplay
#### These will be installed by cmake using the included shell script
* libunwind
* libelf
* libdwarf
* libz
* Egalito

# Recording and Replaying Without sudo
Need to have access to perf to record and replay. By default this can only be done using sudo. To change so that all users can record and replay need to change perf permissions using the following command
* `sudo sh -c 'echo 1 >/proc/sys/kernel/perf_event_paranoid'`

And to persist across restarts
* `sudo sh -c 'echo kernel.perf_event_paranoid=1 > /etc/sysctl.d/local.conf'`

# Building and Installing
'''
git clone <repo_url>
mkdir obj #THIS MUST BE NAMED OBJ. If you change this name you must modify cmake/the download script as well
cd obj
cmake ../rr_muplay
make -j8
'''
* Executable located in ...../obj/bin/rr
