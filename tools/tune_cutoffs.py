#!/usr/bin/env python3
"""Select hybrid cutoffs on a training split and evaluate them on held-out trials."""
from __future__ import annotations
import argparse,csv,math,statistics
from collections import defaultdict
from pathlib import Path

def median(xs): return float(statistics.median(xs))
def geomean(xs):
    vals=[x for x in xs if x>0 and math.isfinite(x)]
    return math.exp(sum(math.log(x) for x in vals)/len(vals)) if vals else math.nan

def read(path:Path):
    with path.open(newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f))
    need={'family','cutoff','pattern','n','trial','input_hash','ns','verified'}
    if not rows or need-set(rows[0]): raise ValueError('missing cutoff benchmark columns')
    out=[]
    for r in rows:
        if r['verified']!='1': raise ValueError('unverified row')
        out.append((r['family'],int(r['cutoff']),r['pattern'],int(r['n']),int(r['trial']),r['input_hash'],float(r['ns'])))
    return out

def tune(rows):
    train=defaultdict(list); test=defaultdict(list)
    for family,cutoff,pattern,n,trial,h,ns in rows:
        del h
        dest=test if trial%3==2 else train
        dest[(family,pattern,n,cutoff)].append(ns)
    cells=sorted({(f,p,n) for f,_,p,n,_,_,_ in rows});result=[]
    for cell in cells:
        f,p,n=cell;candidates=[]
        for key,vals in train.items():
            if key[:3]==cell and vals:candidates.append((median(vals),key[3]))
        if not candidates:continue
        train_med,c=min(candidates);held=test.get((f,p,n,c),[]);base=test.get((f,p,n,1),[])
        result.append({'family':f,'pattern':p,'n':n,'selected_cutoff':c,'train_median_ns':train_med,'heldout_samples':len(held),'heldout_median_ns':median(held) if held else math.nan,'baseline1_median_ns':median(base) if base else math.nan,'heldout_speedup_vs_cutoff1':(median(base)/median(held)) if held and base else math.nan})
    return result

def write(rows,out):
    fields=['family','pattern','n','selected_cutoff','train_median_ns','heldout_samples','heldout_median_ns','baseline1_median_ns','heldout_speedup_vs_cutoff1'];w=csv.DictWriter(out,fieldnames=fields);w.writeheader();w.writerows(rows)

def self_test():
    rows=[]
    for t in range(9):
        for c,time in [(1,100),(8,70),(16,80)]:rows.append(('merge_insertion',c,'random',64,t,str(t),time+(t%2)))
    r=tune(rows);assert len(r)==1 and r[0]['selected_cutoff']==8 and r[0]['heldout_speedup_vs_cutoff1']>1.3;print('PASS: train/held-out cutoff tuning')

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('input',nargs='?',type=Path);ap.add_argument('--output',type=Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
    if a.self_test:self_test();return 0
    if not a.input:ap.error('input required')
    rows=tune(read(a.input))
    if a.output:
        with a.output.open('w',newline='',encoding='utf-8') as f:write(rows,f)
    else:
        import sys;write(rows,sys.stdout)
    return 0
if __name__=='__main__':raise SystemExit(main())
