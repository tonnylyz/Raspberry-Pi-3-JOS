// u64 msyscall(u64 no, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5)
.global msyscall
msyscall:
    cmp x0, #0
    b.eq svc0
    cmp x0, #1
    b.eq svc1
    cmp x0, #2
    b.eq svc2
    cmp x0, #3
    b.eq svc3
    cmp x0, #4
    b.eq svc4
    cmp x0, #5
    b.eq svc5
    cmp x0, #6
    b.eq svc6
    cmp x0, #7
    b.eq svc7
    cmp x0, #8
    b.eq svc8
    cmp x0, #9
    b.eq svc9
    cmp x0, #10
    b.eq svc10
    cmp x0, #11
    b.eq svc11
    cmp x0, #12
    b.eq svc12
    cmp x0, #13
    b.eq svc13
    cmp x0, #14
    b.eq svc14
    cmp x0, #15
    b.eq svc15
    cmp x0, #16
    b.eq svc16
    cmp x0, #17
    b.eq svc17

.macro callsvc svcno
    svc \svcno
    ret
.endm

svc0: callsvc #0
svc1: callsvc #1
svc2: callsvc #2
svc3: callsvc #3
svc4: callsvc #4
svc5: callsvc #5
svc6: callsvc #6
svc7: callsvc #7
svc8: callsvc #8
svc9: callsvc #9
svc10: callsvc #10
svc11: callsvc #11
svc12: callsvc #12
svc13: callsvc #13
svc14: callsvc #14
svc15: callsvc #15
svc16: callsvc #16
svc17: callsvc #17
