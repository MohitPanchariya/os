#pragma once

struct sbi_ret {
    long error;
    long value;
};

// interface exposed by OpenSBI for sbi calls
struct sbi_ret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid);