#!/usr/bin/env python3
"""Evaluate preregistered sorting folklore/hybrid claims using paired raw trials."""
from __future__ import annotations
import argparse,csv,statistics
from collections import defaultdict
from pathlib import Path

CLAIMS=[
 ('insertion_vs_merge','insertion','merge','Insertion beats top-down merge at small n'),
 ('binary_vs_linear_insertion','binary_insertion','insertion','Binary insertion trades comparisons for extra control overhead'),
 ('merge_hybrid_vs_merge','merge_insertion_24','merge','Insertion leaves improve mergesort'),
 ('quick_hybrid_vs_quick','quick_insertion_24','quick_median3','Insertion leaves improve median-of-three quicksort'),
 ('threeway_vs_twoway','quick_3way','quick_hoare','Three-way partitioning helps duplicate-heavy inputs'),
 ('radix_digit_width','radix_lsd_11','radix_lsd','Radix digit width has a hardware/input-size crossover'),
 ('natural_vs_fixed_merge','natural_merge','merge','Run-adaptive merging helps structured inputs'),
]

def read(path:Path):
 with path.open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
 need={'algorithm','pattern','n','trial','input_hash','ns','verified'}
 if not rows or need-set(rows[0]):raise ValueError('missing scalar raw columns')
 out=defaultdict(dict)
 for r in rows:
  if r['verified']!='1':raise ValueError('unverified row')
  key=(r['pattern'],int(r['n']),int(r['trial']),r['input_hash']);out[key][r['algorithm']]=float(r['ns'])
 return out

def evaluate(raw):
 grouped=defaultdict(list)
 for (pattern,n,trial,h),peers in raw.items():
  del trial,h
  for cid,challenger,reference,text in CLAIMS:
   if challenger in peers and reference in peers and peers[challenger]>0:grouped[(cid,text,challenger,reference,pattern,n)].append(peers[reference]/peers[challenger])
 rows=[]
 for key,ratios in sorted(grouped.items()):
  cid,text,ch,ref,p,n=key;med=float(statistics.median(ratios));wins=sum(x>1 for x in ratios);ties=sum(x==1 for x in ratios)
  rows.append({'claim_id':cid,'claim':text,'challenger':ch,'reference':ref,'pattern':p,'n':n,'paired_samples':len(ratios),'median_speedup':med,'win_rate':wins/len(ratios),'ties':ties,'direction_supported':int(med>1)})
 return rows

def write(rows,out):
 fields=['claim_id','claim','challenger','reference','pattern','n','paired_samples','median_speedup','win_rate','ties','direction_supported'];w=csv.DictWriter(out,fieldnames=fields);w.writeheader();w.writerows(rows)

def self_test():
 raw={('random',8,0,'a'):{'insertion':5,'merge':10},('random',8,1,'b'):{'insertion':6,'merge':12}};rows=evaluate(raw);r=next(x for x in rows if x['claim_id']=='insertion_vs_merge');assert r['median_speedup']==2 and r['win_rate']==1;print('PASS: preregistered paired claim matrix')

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('input',nargs='?',type=Path);ap.add_argument('--output',type=Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test:self_test();return 0
 if not a.input:ap.error('input required')
 rows=evaluate(read(a.input))
 if a.output:
  with a.output.open('w',newline='',encoding='utf-8') as f:write(rows,f)
 else:
  import sys;write(rows,sys.stdout)
 return 0
if __name__=='__main__':raise SystemExit(main())
