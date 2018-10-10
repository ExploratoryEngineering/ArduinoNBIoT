# The Telenor NB-IoT Arduino library

This is a library to make the NB-IoT breakouts from
[Exploratory Engineering](https://shop.exploratory.engineering/) a bit easier to use.

It integrates with the backend service at https://nbiot.engineering/.
The `sendBytes` and `sendString` functions can be used to send data to remote servers.

## Installing the library

Click the green «Clone or Download» button, then «Download ZIP». Add it in the
Arduino IDE with `Sketch|Library|Add .ZIP library`. The library will now be
available via Library Manager.

## Troubleshooting

If you are having problems getting the module to work you can connect it
directly to a serial port or to an USB-to-serial adapter.

First make sure that your module is set up to automatically connect and that the
scrambling parameter is set to TRUE:

```text
AT+NCONFIG?

+NCONFIG: "AUTOCONNECT","TRUE"
+NCONFIG: "CR_0354_0338_SCRAMBLING","TRUE"
+NCONFIG: "CR_0859_SI_AVOID","TRUE"
+NCONFIG: "COMBINE_ATTACH","FALSE"
+NCONFIG: "CELL_RESELECTION","FALSE"
+NCONFIG: "ENABLE_BIP","FALSE"

OK
```

If `AUTOCONNECT` or `CR_0354_0338_SCRAMBLING` is set to false you can set it
by issuing the following commands which will update the settings and reboot the
module:

```text
AT+NCONFIG="AUTOCONNECT","TRUE"
AT+NCONFIG="CR_0354_0338_SCRAMBLING","TRUE"
AT+NRB
```

These settings will be stored in NVRAM and are kept even when the power is off.

If that looks OK you can check out the radio status by using the `AT+NUESTATS`
command:

```text
AT+NUESTATS

"Signal power",-778
"Total power",-706
"TX power",230
"TX time",28424
"RX time",86269
"Cell ID",16964199
"ECL",0
"SNR",135
"EARFCN",6352
"PCI",6
"RSRQ",-108

OK
```

If f.e. "TX time" and "RX time" is zero the radio and data should be turned on:

```text
AT+CFUN=1
OK
AT+CGATT=1
OK
```

You can verify connectivity by issuing the NPING command:

```text
AT+NPING="8.8.8.8"
OK

+NPING: "8.8.8.8",51,3586
OK
```

## Missing features

* There's no sanity check on firmware versions. Older versions of the firmware
  aren't compatible with the library since the AT command syntax is different.
* No support for URCs (aka Unsolicited Response Codes) to detect incomning
  packets, connectivity change and so on.
