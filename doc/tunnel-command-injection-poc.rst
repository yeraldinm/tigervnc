Tunnel Command Injection PoC
============================

This document describes a proof-of-concept demonstrating the
command injection vulnerability that existed in `vncviewer`'s
SSH tunnel handling prior to commit 7541601f.

Background
---------

`createTunnel()` constructed an SSH command using environment
variables for the gateway and remote host values. These were
expanded by the shell when running `system()`. Because the
parameters were not validated, special shell syntax such as
command substitution could be injected via the `-via` parameter.

Proof of concept
----------------

Build `vncviewer` from a commit prior to 7541601f and run:

.. code-block:: bash

   vncviewer -via '$(touch /tmp/vuln)' localhost:1

During tunnel setup the `touch` command executes, creating the
file `/tmp/vuln`. The fixed version rejects this argument and
prints ``Unsafe characters in host specification``.
