#!/usr/bin/env python3
"""Run a sort benchmark and capture a reproducibility/host-control manifest."""
from __future__ import annotations
import argparse,datetime as dt,hashlib,json,os,platform,subprocess,sys,time
from pathlib import Path

def command_output(command:list[str])->str|None:
 try:return subprocess.run(command,check=True,text=True,capture_output=True).stdout.strip()
 except (OSError,subprocess.CalledProcessError):return None

def read_text(path:str)->str|None:
 try:return Path(path).read_text(encoding='utf-8').strip()
 except OSError:return None

def host_controls()->dict:
 out={'loadavg':list(os.getloadavg()) if hasattr(os,'getloadavg') else None}
 if hasattr(os,'sched_getaffinity'):
  try:out['affinity']=sorted(os.sched_getaffinity(0))
  except OSError:out['affinity']=None
 if platform.system()=='Linux':
  out.update({'lscpu':command_output(['lscpu','--json']),'perf_event_paranoid':read_text('/proc/sys/kernel/perf_event_paranoid'),'cpu0_scaling_governor':read_text('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor'),'cpu0_energy_performance_preference':read_text('/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference'),'transparent_hugepage':read_text('/sys/kernel/mm/transparent_hugepage/enabled')})
 elif platform.system()=='Darwin':
  out.update({'cpu_brand':command_output(['sysctl','-n','machdep.cpu.brand_string']),'physical_cpu':command_output(['sysctl','-n','hw.physicalcpu']),'logical_cpu':command_output(['sysctl','-n','hw.logicalcpu']),'memsize':command_output(['sysctl','-n','hw.memsize']),'thermal':command_output(['pmset','-g','therm'])})
 return out

def main()->int:
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cpu',type=int,help='pin benchmark child to one CPU where os.sched_setaffinity is available');p.add_argument('--settle-ms',type=int,default=0,help='sleep before launching the measured process');p.add_argument('binary',type=Path);p.add_argument('output_dir',type=Path);p.add_argument('benchmark_args',nargs=argparse.REMAINDER,help='arguments passed to benchmark; prefix with --');a=p.parse_args();binary=a.binary.resolve()
 if not binary.exists():p.error(f'binary does not exist: {binary}')
 if a.settle_ms<0:p.error('--settle-ms must be nonnegative')
 if a.cpu is not None and not hasattr(os,'sched_setaffinity'):p.error('--cpu is unsupported on this platform')
 a.output_dir.mkdir(parents=True,exist_ok=True);raw_path=a.output_dir/'raw.csv';bench=list(a.benchmark_args)
 if bench and bench[0]=='--':bench=bench[1:]
 command=[str(binary),*bench];preexec=None
 if a.cpu is not None:
  cpu=a.cpu
  def pin():os.sched_setaffinity(0,{cpu})
  preexec=pin
 if a.settle_ms:time.sleep(a.settle_ms/1000)
 before=host_controls()
 with raw_path.open('w',encoding='utf-8',newline='') as raw:completed=subprocess.run(command,stdout=raw,stderr=sys.stderr,preexec_fn=preexec)
 after=host_controls()
 if completed.returncode!=0:return completed.returncode
 raw_digest=hashlib.sha256(raw_path.read_bytes()).hexdigest();binary_digest=hashlib.sha256(binary.read_bytes()).hexdigest();binary_env=command_output([str(binary),'--environment'])
 manifest={'schema_version':2,'captured_at_utc':dt.datetime.now(dt.timezone.utc).isoformat(),'command':command,'requested_cpu':a.cpu,'settle_ms':a.settle_ms,'raw_csv':raw_path.name,'raw_csv_sha256':raw_digest,'binary_sha256':binary_digest,'benchmark_environment':json.loads(binary_env) if binary_env else None,'host':{'platform':platform.platform(),'system':platform.system(),'release':platform.release(),'machine':platform.machine(),'processor':platform.processor(),'logical_cpu_count':os.cpu_count(),'python':platform.python_version(),'controls_before':before,'controls_after':after,'relevant_environment':{k:os.environ.get(k) for k in ['OMP_NUM_THREADS','TBB_NUM_THREADS','MALLOC_CONF','GLIBC_TUNABLES'] if os.environ.get(k) is not None}},'source':{'git_commit':command_output(['git','rev-parse','HEAD']),'git_status_porcelain':command_output(['git','status','--porcelain'])}}
 manifest_path=a.output_dir/'manifest.json';manifest_path.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n',encoding='utf-8');print(f'wrote {raw_path} ({raw_digest})',file=sys.stderr);print(f'wrote {manifest_path}',file=sys.stderr);return 0
if __name__=='__main__':raise SystemExit(main())
