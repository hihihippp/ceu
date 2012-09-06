#include <string.h>
#include <limits.h>

#define PR_MAX 0x7F
#define PR_MIN (-0x7F)

#define PTR(str,tp) ((tp)(CEU->mem + str ))
#define TRK(t,p,l,e) {t.prio=p; t.lbl=l; t.emt=e;}  // TODO: CEU_SIMUL

#define N_MEM       (=== N_MEM ===)
#define N_TRACKS    (=== N_TRACKS ===)
#define CEU_WCLOCK0 (=== CEU_WCLOCK0 ===)
#define CEU_ASYNC0  (=== CEU_ASYNC0 ===)
#define CEU_EMIT0   (=== CEU_EMIT0 ===)
#define CEU_FIN0    (=== CEU_FIN0 ===)

#ifdef CEU_SIMUL
#define N_LABELS    (=== N_LABELS ===)
#endif

// Macros that can be defined:
// ceu_out_pending() (1)
// ceu_out_wclock(us)
// ceu_out_event(id, len, data)

typedef === TCEU_OFF === tceu_off;
typedef === TCEU_LBL === tceu_lbl;

=== DEFS ===

typedef struct {
    s32 togo;
#ifdef CEU_SIMUL
    u32 ext;
#endif
    tceu_lbl lbl;
} tceu_wclock;

typedef struct {
#ifdef CEU_TRK_PRIO
    s8 prio;
#endif
    tceu_lbl lbl;
    tceu_lbl emt;
} tceu_trk;

enum {
=== LABELS ===
};

int ceu_go (int* ret);

=== HOST ===

typedef struct {
    int             tracks_n;
#ifdef CEU_TRK_PRIO
    tceu_trk        tracks[N_TRACKS+1];  // 0 is reserved
#else
    tceu_trk        tracks[N_TRACKS];
#endif

#ifdef CEU_EXTS
    void*           ext_data;
    int             ext_int;
#endif

#ifdef CEU_WCLOCKS
    int             wclk_late;
    tceu_wclock*    wclk_cur;
#ifdef CEU_SIMUL
    u32             wclk_ext;
    s8              wclk_any;
#endif
#endif

#ifdef CEU_ASYNCS
    int             async_cur;
#endif

#ifdef CEU_SIMUL
#endif
    tceu_trk        trk;        // TODO: CEU_SIMUL
    char            mem[N_MEM];
} tceu;

#ifdef CEU_SIMUL
#include "simul.h"
#endif

tceu CEU_ = { 0, 0,
#ifdef CEU_EXTS
    0, 0,
#endif
#ifdef CEU_WCLOCKS
    0, 0,
#ifdef CEU_SIMUL
    0, 0,
#endif
#endif
#ifdef CEU_ASYNCS
    0,
#endif
#ifdef CEU_SIMUL
    0,
#endif
    0
};
tceu* CEU = &CEU_;

/**********************************************************************/

#ifdef CEU_TRK_PRIO
    #ifdef CEU_TRK_CHK
        #define ceu_track_ins(chk,prio,lbl,emt) ceu_track_ins_YY(chk,prio,lbl,emt)
        void ceu_track_ins_YY (int chk, s8 prio, tceu_lbl lbl, tceu_lbl emt)
    #else
        #define ceu_track_ins(chk,prio,lbl,emt) ceu_track_ins_NY(prio,lbl,emt)
        void ceu_track_ins_NY (s8 prio, tceu_lbl lbl, tceu_lbl emt)
    #endif
#else
    #ifdef CEU_TRK_CHK
        #define ceu_track_ins(chk,prio,lbl,emt) ceu_track_ins_YN(chk,lbl,emt)
        void ceu_track_ins_YN (int chk, tceu_lbl lbl, tceu_lbl emt)
    #else
        #define ceu_track_ins(chk,prio,lbl,emt) ceu_track_ins_NN(lbl,emt)
        void ceu_track_ins_NN (tceu_lbl lbl, tceu_lbl emt)
    #endif
#endif
{
#ifdef CEU_SIMUL
    if (prio >= 0)
        ceu_sim_state_path(CEU->trk.lbl, lbl);
    else
        ceu_sim_state_path(CEU->trk.lbl, PTR(CEU_EMIT0,tceu_lbl*)[lbl]);
#endif
#ifdef CEU_TRK_CHK
    {int i;
    if (chk) {
        for (i=1; i<=CEU->tracks_n; i++) {   // TODO: avoids (lbl vs emt_gte)
            if (lbl==CEU->tracks[i].lbl && prio==CEU->tracks[i].prio) {
#ifdef CEU_SIMUL
                S.needsChk = 1;
#endif
                return;
            }
        }
    }}
#endif

#ifdef CEU_TRK_PRIO
    {int i;
    for (i=++CEU->tracks_n; (i>1) && (prio>CEU->tracks[i/2].prio); i/=2)
        CEU->tracks[i] = CEU->tracks[i/2];
    CEU->tracks[i].prio = prio;
    CEU->tracks[i].emt = emt;
    CEU->tracks[i].lbl  = lbl;}
#else
    CEU->tracks[CEU->tracks_n].emt = emt;
    CEU->tracks[CEU->tracks_n++].lbl = lbl;
#endif

#ifdef CEU_SIMUL
    if (CEU->tracks_n > S.n_tracks)
        S.n_tracks = CEU->tracks_n;
#endif
}

int ceu_track_rem (tceu_trk* trk)
{
    if (CEU->tracks_n == 0)
        return 0;

#ifdef CEU_TRK_PRIO
    {int i,cur;
    tceu_trk* last;

    if (trk)
        *trk = CEU->tracks[1];

    last = &CEU->tracks[CEU->tracks_n--];

    for (i=1; i*2<=CEU->tracks_n; i=cur)
    {
        cur = i * 2;
        if (cur!=CEU->tracks_n && CEU->tracks[cur+1].prio>CEU->tracks[cur].prio)
            cur++;

        if (CEU->tracks[cur].prio>last->prio)
            CEU->tracks[i] = CEU->tracks[cur];
        else
            break;
    }
    CEU->tracks[i] = *last;
    return 1;}
#else
    *trk = CEU->tracks[--CEU->tracks_n];
    return 1;
#endif
}

void ceu_spawn (tceu_lbl* lbl, tceu_lbl emt)
{
    if (*lbl != Inactive) {
        ceu_track_ins(0, PR_MAX, *lbl, emt);
        *lbl = Inactive;
    }
}

void ceu_trigger (tceu_off off, tceu_lbl emt)
{
    int i;
    int n = CEU->mem[off];
    for (i=0 ; i<n ; i++) {
        ceu_spawn((tceu_lbl*)&CEU->mem[off+1+(i*sizeof(tceu_lbl))], emt);
    }
}

/**********************************************************************/

#ifdef CEU_EXTS
// returns a pointer to the received value
int* ceu_ext_f (int v) {
    CEU->ext_int = v;
    return &CEU->ext_int;
}
#endif

#ifdef CEU_EMITS
int ceu_track_peek (tceu_trk* trk)
{
    *trk = CEU->tracks[1];
    return CEU->tracks_n > 0;
}
#endif

#ifdef CEU_FINS
void ceu_fins (int i, int j)
{
    for (; i<j; i++) {
        tceu_lbl* fin0 = PTR(CEU_FIN0,tceu_lbl*);
        if (fin0[i] != Inactive)
            ceu_track_ins(0, PR_MAX-i, fin0[i], 0);
    }
}
#endif

#ifdef CEU_WCLOCKS

#define CEU_WCLOCK_NONE LONG_MAX
#ifdef CEU_SIMUL
#define CEU_WCLOCK_ANY (LONG_MAX-1)
#endif

#ifdef CEU_SIMUL
int ceu_wclock_lt (tceu_wclock* tmr) {
    return 0;
}
#else
int ceu_wclock_lt (tceu_wclock* tmr) {
    if (!CEU->wclk_cur || tmr->togo<CEU->wclk_cur->togo) {
        CEU->wclk_cur = tmr;
        return 1;
    }
    return 0;
}
#endif

void ceu_wclock_enable (int idx, s32 us, tceu_lbl lbl) {
    tceu_wclock* tmr = &(PTR(CEU_WCLOCK0,tceu_wclock*)[idx]);
    s32 dt = us - CEU->wclk_late;
#ifdef ceu_out_wclock
    int nxt;
#endif

    tmr->togo = dt;
#ifdef CEU_SIMUL
    tmr->togo = (CEU->wclk_any ? CEU_WCLOCK_ANY : dt);
    tmr->ext  = CEU->wclk_ext;
#endif
    tmr->lbl  = lbl;

#ifdef ceu_out_wclock
    nxt = ceu_wclock_lt(tmr);
#else
    ceu_wclock_lt(tmr);
#endif

#ifdef ceu_out_wclock
    if (nxt)
        ceu_out_wclock(dt);
#endif
}

#endif

/**********************************************************************/

int ceu_go_init (int* ret)
{
#ifdef CEU_SIMUL
    TRK(CEU->trk, 0,0,0);
#endif
    ceu_track_ins(0, PR_MAX, Init, 0);
    return ceu_go(ret);
}

#ifdef CEU_EXTS
int ceu_go_event (int* ret, int id, void* data)
{
    CEU->ext_data = data;
    ceu_trigger(id, 0);

#ifdef CEU_SIMUL
    TRK(CEU->trk, 0,0,0);
#ifdef CEU_WCLOCKS
    CEU->wclk_ext++;
    CEU->wclk_any = 0;
#endif
    return 0;
#else
#ifdef CEU_WCLOCKS
    CEU->wclk_late--;
#endif
    return ceu_go(ret);
#endif
}
#endif

#ifdef CEU_ASYNCS
int ceu_go_async (int* ret, int* pending)
{
    int i,s=0;

    tceu_lbl* ASY0 = PTR(CEU_ASYNC0,tceu_lbl*);

    for (i=0; i<CEU_ASYNCS; i++) {
        int idx = (CEU->async_cur+i) % CEU_ASYNCS;
        if (ASY0[idx] != Inactive) {
#ifdef CEU_SIMUL
            CEU_SIMUL_PRE(0);
            TRK(CEU->trk,0,0,0);
            tceu_lbl* ASY0 = PTR(CEU_ASYNC0,tceu_lbl*);
#endif
            ceu_track_ins(0, PR_MAX, ASY0[idx], 0);
            ASY0[idx] = Inactive;
            CEU->async_cur = (idx+1) % CEU_ASYNCS;
#ifdef CEU_SIMUL
#ifdef CEU_WCLOCKS
            CEU->wclk_ext++;
            CEU->wclk_any = 0;
#endif
            CEU_SIMUL_POS();
            s = 0;   // don't break (all at once)
#else
#ifdef CEU_WCLOCKS
            CEU->wclk_late--;
#endif
            s = ceu_go(ret);
            break;
#endif
        }
    }

    if (pending != NULL) {
        for (i=0; i<CEU_ASYNCS; i++) {
            if (ASY0 != Inactive) {
                *pending = 1;
                break;
            }
        }
    }

    return s;
}
#endif

int ceu_go_wclock (int* ret, s32 dt)
{
#ifdef CEU_WCLOCKS
    int i;
    s32 min_togo = CEU_WCLOCK_NONE;
#ifdef CEU_SIMUL
    u32 min_ext = 0;
    TRK(CEU->trk, 0,0,0);
#endif

    tceu_wclock* CLK0 = PTR(CEU_WCLOCK0,tceu_wclock*);

    if (!CEU->wclk_cur)
        return 0;

    if (CEU->wclk_cur->togo <= dt) {
        min_togo = CEU->wclk_cur->togo;
        CEU->wclk_late = dt - CEU->wclk_cur->togo;   // how much late the wclock is
#ifdef CEU_SIMUL
        min_ext = CEU->wclk_cur->ext;
        CEU->wclk_late = 0;
#endif
    }

    // spawns all togo/ext
    // finds the next CEU->wclk_cur
    // decrements all togo
    CEU->wclk_cur = NULL;

#ifdef CEU_SIMUL
    if (dt != CEU_WCLOCK_ANY)
#endif
    for (i=0; i<CEU_WCLOCKS; i++)
    {
        tceu_wclock* tmr = &CLK0[i];
        if (tmr->lbl == Inactive)
            continue;

#ifdef CEU_SIMUL
        if (tmr->togo == CEU_WCLOCK_ANY)
            continue;
        if ( tmr->togo==min_togo && tmr->ext==min_ext) {
            tmr->togo = 0;
#else
        if ( tmr->togo==min_togo ) {
#endif
            ceu_spawn(&tmr->lbl, 0);           // spawns sharing phys/ext
        } else {
            tmr->togo -= dt;
            ceu_wclock_lt(tmr);             // next? (sets CEU->wclk_cur)
        }
    }

#ifdef CEU_SIMUL
    // traverse CEU_WCLOCK_ANY
    u8 arr[CEU_WCLOCKS];
    u8 arr_n = 0;
    for (int i=0; i<CEU_WCLOCKS; i++)
    {
        tceu_wclock* tmr = &CLK0[i];
        if (tmr->togo==CEU_WCLOCK_ANY && tmr->ext==min_ext)
            arr[arr_n++] = i;
    }
fprintf(stderr,"N=%d\n",arr_n);
    for (int i=1; i<(1<<arr_n); i++)
    {
fprintf(stderr,"i=%d\n",i);
        CEU_SIMUL_PRE(1);
        for (int j=0; j<arr_n; j++) {
            if (i & (1<<j)) {
                tceu_wclock* _tmr = &(PTR(CEU_WCLOCK0,tceu_wclock*)[ arr[j] ]);
                ceu_spawn(&_tmr->lbl, 0);
                _tmr->togo = 0;
            }
        }
        CEU_SIMUL_POS();
    }

    return 0;
#else

#ifdef ceu_out_wclock
    if (CEU->wclk_cur)
        ceu_out_wclock(CEU->wclk_cur->togo);
    else
        ceu_out_wclock(CEU_WCLOCK_NONE);
#endif
    {int s = ceu_go(ret);
    return s;}

#endif

#else
    return 0;
#endif
}

int ceu_go (int* ret)
{
    tceu_trk trk;
    tceu_lbl _lbl_;

#ifdef CEU_EMITS
    int _step_ = PR_MIN;
#endif

    while (ceu_track_rem(&trk))
    {
#ifdef CEU_EMITS
        if (trk.prio < 0) {
            tceu_lbl* EMT0 = PTR(CEU_EMIT0,tceu_lbl*);
            tceu_lbl T[N_TRACKS+1];
            int n = 0;
            _step_ = trk.prio;
            while (1) {
                tceu_lbl lbl = EMT0[trk.lbl]; // trk.lbl is actually a off
                if (lbl != Inactive)
                    T[n++] = lbl;
                if (!ceu_track_peek(&trk) || (trk.prio < _step_))
                    break;
                else
                    ceu_track_rem(NULL);
            }
#ifdef CEU_SIMUL
            //CEU->trk = {0,0,0};
#endif
            for (;n>0;)
                ceu_track_ins(0, PR_MAX, T[--n], trk.emt);
            continue;
        } else
#endif
#ifdef CEU_SIMUL
        CEU->trk = trk;
#endif
        _lbl_ = trk.lbl;
_SWITCH_:
fprintf(stderr,"TRK: %d\n", _lbl_);

        switch (_lbl_)
        {
            case Init:
    === CODE ===
        }
    }

    return 0;
}

int ceu_go_all ()
{
    int ret = 0;
#ifdef CEU_ASYNCS
    int async_cnt;
#endif

    if (ceu_go_init(&ret))
        return ret;

#ifdef IN_Start
    //*PVAL(int,IN_Start) = (argc>1) ? atoi(argv[1]) : 0;
    if (ceu_go_event(&ret, IN_Start, NULL))
        return ret;
#endif

#ifdef CEU_ASYNCS
    for (;;) {
        if (ceu_go_async(&ret,&async_cnt))
            return ret;
        if (async_cnt == 0)
            break;              // returns nothing!
    }
#endif

    return ret;
}
