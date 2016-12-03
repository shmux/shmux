> The most likely way for the world to be destroyed, most experts
> agree, is by accident.  That's where we come in.  We're computer
> professionals.  We cause accidents.

## What is shmux?

*shmux* is program for executing the same command on many hosts in
parallel.  For each target, a child process is spawned by *shmux*, and
a shell on the target obtained one of the supported methods: rsh, ssh,
or sh.  The output produced by the children is received by *shmux* and
either (optionally) output in turn to the user using an easy to read
format, or written to files for later processing making it well suited
for use in scripts.

For more details, check out the
[shmux(1)](http://web.taranis.org/shmux/man/shmux.1.html) manual page.
The example shown below is also a good illustration of some of
*shmux*'s capabilities and is dissected [frame by
frame](http://web.taranis.org/shmux/example/) for your convenience.

![Sample shmux output](http://web.taranis.org/shmux/shmux.gif)

## Features

*shmux* solves a fairly simple problem that can be addressed with a
few lines of shell or Perl.  This may lead you to think that using
*shmux* is total overkill, but *shmux* is a powerful tool that offers
many time and life saving features, so read on!

* **When used in a script**
  * Ability to define what is and what is not an error for the command
    being run (based on exit code and output content)
  * Output and exit codes are saved into files to facilitate use from
    a script.
* **When used interactively** (directly on the command line, or from
within a script/wrapper):
  * Well formatted output
  * Standard error output displayed in bold
  * Real-time status shown
  * Ability to pause, resume, quit cleanly
  * Automatically pause on error, allowing the user to cleanly stop
    before more goes wrong
  * Ability to hide output of successful targets
  * Mixed or un-mixed target outputs

##  Related Work

Now..

Where were all these when i wrote *shmux*?
why do people keep reinventing the wheel?

* [dsh](http://www.netfort.gr.jp/~dancer/software/dsh.html)
* [dssh](http://dssh.subverted.net/)
* [fanout](http://www.stearns.org/fanout/)
* [Kees Cook's gsh](http://outflux.net/unix/software/gsh/)
* [Mr. Shell](http://www.voltar-confed.org/mrsh/)
* [multi-rsh](http://hea-www.harvard.edu/~fine/Tech/multi-rsh.html)
* [multixterm](http://expect.nist.gov/example/multixterm.man.html)
* [mussh](http://sourceforge.net/projects/mussh/)
* [pdsh](https://computing.llnl.gov/linux/pdsh.html)
* [pssh](http://www.theether.org/pssh/)
* [p-run](http://www.tuxrocks.com/Projects/p-run/)
* [PyDsh](http://sourceforge.net/projects/pydsh/)
* [remote_update.pl](http://store.z-kat.com/~valankar/)
* [RGANG](http://fermitools.fnal.gov/abstracts/rgang/abstract.html)
* [rshall](http://www.occam.com/tools/)
* [tentakel](http://tentakel.biskalar.de/)
* [vxargs](http://dharma.cis.upenn.edu/planetlab/vxargs/)
