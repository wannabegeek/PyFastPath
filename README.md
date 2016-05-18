PyFastPath
======

Python bindings for using the FastPath client libraries [https://github.com/wannabegeek/DCM]

Installation
------------

**On Unix (Linux, OS X)**

 - clone this repository
 - `pip install ./PyFastPath`

Runtime
----------
You will need to make sure the `libfastpath.so` is in your LD_LIBRARY_PATH

Test call
---------

```python
import fastpath
m = fastpath.MutableMessage()
m.setSubject("SOME.TEST.MESSAGE")
m.addStringField("Name", "Tom")
```
