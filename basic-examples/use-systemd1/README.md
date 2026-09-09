## How to use
```bash
cd use-systemd1
./get_service_status_cmd
./get_service_status
```

## Equivalent dbus command
```bash
busctl call -j \
org.freedesktop.systemd1 \
/org/freedesktop/systemd1 \
org.freedesktop.systemd1.Manager \
ListUnitsByNames s \
<service name>
```

## About `org.freedesktop.systemd1.service`
I have presented ways to get service status via `org.freedesktop.systemd1.service`.
\
But `org.freedesktop.systemd1.service` is way more powerful. We can also see what it can do via introspect (eg: EnableUnitFiles asbb, StartUnit ss, StopUnit ss, RestartUnit ss, TryRestartUnit ss, ...).
```bash
busctl introspect org.freedesktop.systemd1 /org/freedesktop/systemd1
```

## Some more notes about `org.freedesktop.systemd1.service`
Usually, this service is active. However, there would have couple of reasons this basic service won't be active.

\
The first thing we can check is on the dbus whether exists object running.
```bash
busctl tree org.freedesktop.systemd1
```
If not, dbus will show
```
$ busctl tree org.freedesktop.systemd1
Failed to introspect object / of service org.freedesktop.systemd1: Launch helper exited with unknown return code 1
No objects discovered.
```
Then we can cat the service file, and if the `Exec=/bin/false` appear means the service was been disable.
\
A normal systemd service file shall have `Exec=/lib/systemd/systemd` 
```bash
$ cat /usr/share/dbus-1/system-services/org.freedesktop.systemd1.service
#  SPDX-License-Identifier: LGPL-2.1-or-later
#
#  This file is part of systemd.
#
#  systemd is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation; either version 2.1 of the License, or
#  (at your option) any later version.

[D-BUS Service]
Name=org.freedesktop.systemd1
Exec=/bin/false
User=root
```
Finally, we can also run this to see what your PID 1 is.
\
If it’s not systemd, then you're likely in a container or non-systemd environment. That means no way to use systemd.
```bash
ps -p 1 -o comm=
```
