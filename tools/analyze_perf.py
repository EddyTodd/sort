#!/usr/bin/env python3
"""Reduce sort_perf hardware-counter trials into robust per-cell summaries."""
from __future__ import annotations
import argparse,csv,math,statistics
from collections import defaultdict
from pathlib import Path

def med(xs):return float(statistics.median(xs)) if xs else math.nan
def read(path:Path):
 with path.open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
 need={'algorithm','pattern','n','trial','ns','perf_available','cycles','instructions','branches','branch_misses','cache_references','cache_misses','verified'}
 if not rows or need-set(rows[0]):raise ValueError('missing perf columns')
 for r in rows:
  if r['verified']!='1':raise ValueError('unverified row')
 return rows

def summarize(rows):
 g=defaultdict(list)
 for r in rows:g[(r['algorithm'],r['pattern'],int(r['n']))].append(r)
 out=[]
 for (a,p,n),rs in sorted(g.items()):
  ns=[float(r['ns']) for r in rs];valid=[r for r in rs if r['perf_available']=='1'];cyc=[float(r['cycles']) for r in valid];ins=[float(r['instructions']) for r in valid];br=[float(r['branches']) for r in valid];bm=[float(r['branch_misses']) for r in valid];cr=[float(r['cache_references']) for r in valid];cm=[float(r['cache_misses']) for r in valid];cpi=[c/i for c,i in zip(cyc,ins) if i>0];bmr=[m/b for m,b in zip(bm,br) if b>0];cmr=[m/r for m,r in zip(cm,cr) if r>0]
  out.append({'algorithm':a,'pattern':p,'n':n,'samples':len(rs),'perf_samples':len(valid),'median_ns':med(ns),'median_cycles':med(cyc),'median_instructions':med(ins),'median_cpi':med(cpi),'median_branches':med(br),'median_branch_miss_rate':med(bmr),'median_cache_references':med(cr),'median_cache_miss_rate':med(cmr)})
 return out

def write(rows,out):
 fields=['algorithm','pattern','n','samples','perf_samples','median_ns','median_cycles','median_instructions','median_cpi','median_branches','median_branch_miss_rate','median_cache_references','median_cache_miss_rate'];w=csv.DictWriter(out,fieldnames=fields);w.writeheader();w.writerows(rows)

def self_test():
 rows=[{'algorithm':'a','pattern':'random','n':'10','trial':'0','ns':'100','perf_available':'1','cycles':'200','instructions':'100','branches':'20','branch_misses':'2','cache_references':'10','cache_misses':'1','verified':'1'}];s=summarize(rows)[0];assert s['median_cpi']==2 and abs(s['median_branch_miss_rate']-.1)<1e-9;print('PASS: hardware-counter reduction')
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('input',nargs='?',type=Path);ap.add_argument('--output',type=Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test:self_test();return 0
 if not a.input:ap.error('input required')
 rows=summarize(read(a.input))
 if a.output:
  with a.output.open('w',newline='',encoding='utf-8') as f:write(rows,f)
 else:
  import sys;write(rows,sys.stdout)
 return 0
if __name__=='__main__':raise SystemExit(main())
