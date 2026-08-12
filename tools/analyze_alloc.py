#!/usr/bin/env python3
"""Summarize allocation-only experiments from sort_alloc."""
from __future__ import annotations
import argparse,csv,statistics
from collections import defaultdict
from pathlib import Path

def read(path:Path):
 with path.open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
 need={'algorithm','pattern','n','allocation_calls','total_requested_bytes','peak_live_bytes','max_single_allocation_bytes','live_bytes_at_stop','verified'}
 if not rows or need-set(rows[0]):raise ValueError('missing allocation columns')
 for r in rows:
  if r['verified']!='1':raise ValueError('unverified row')
 return rows

def summarize(rows):
 g=defaultdict(list)
 for r in rows:g[(r['algorithm'],r['pattern'],int(r['n']))].append(r)
 out=[]
 for (a,p,n),rs in sorted(g.items()):
  m=lambda k:float(statistics.median(int(r[k]) for r in rs))
  out.append({'algorithm':a,'pattern':p,'n':n,'samples':len(rs),'median_allocation_calls':m('allocation_calls'),'median_total_requested_bytes':m('total_requested_bytes'),'median_peak_live_bytes':m('peak_live_bytes'),'median_max_single_allocation_bytes':m('max_single_allocation_bytes'),'max_live_bytes_at_stop':max(int(r['live_bytes_at_stop']) for r in rs)})
 return out

def write(rows,out):
 fields=['algorithm','pattern','n','samples','median_allocation_calls','median_total_requested_bytes','median_peak_live_bytes','median_max_single_allocation_bytes','max_live_bytes_at_stop'];w=csv.DictWriter(out,fieldnames=fields);w.writeheader();w.writerows(rows)

def self_test():
 rows=[{'algorithm':'m','pattern':'r','n':'10','allocation_calls':'1','total_requested_bytes':'80','peak_live_bytes':'80','max_single_allocation_bytes':'80','live_bytes_at_stop':'0','verified':'1'}];assert summarize(rows)[0]['median_peak_live_bytes']==80;print('PASS: allocation reduction')
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
