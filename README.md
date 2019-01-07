# m3

A proxy-based multi-path transfer tool works as an application, deprecates _mpflex-hsr_.

## Build & Install

Barely `make && make install` builds and installs m3. To uninstall, use `make uninstall`. By default, the executable files are installed to `/usr/bin`. You can specify some `Makefile` variables to configure your building/installing process.

- `INSTALL_PATH`

    For `make install`, this overrides the default install path.

Note: Our makefile works with `makedepend`. `make` will not fail without `makedepend`, but file dependency may not be correctly handled in this way. For example, you may need to rebuild the project if some header files are modified.  
If failed building for lacking of some tools/libraries, run the `requirements.sh` (with apt repo manager)

## `m3` Config File Format

`m3` accepts a [JSON](http://www.json.org/) script file as data input, the `m3` program will then search for fields in that file to configure itself. In this program we are using (`cJSON`)[https://github.com/DaveGamble/cJSON] to parse JSON, so install this library before using.

We now describe the fields that we use for configuration below:

- Fields

    Fields are members of the root JSON object in the script file. We have _exactly_ two kinds of fields:

    * Critial Fields

        A `m3` config file MUST contain all _critical fields_, or the program will refuse to execute.

    * Optional Fields

        A `m3` config file MAY contain any _optional fields_, these fields come with a default value, value specified in the config file simply overwrites them.
    
    * Subfields

        We support the usage of nesting JSON objects in `m3` config file. Subfields are members of JSON object fields. Like fields, subfields are also either _critical_ or _optional_. The script below shows one _field_ and two _subfields_:

        ```json
        {
            "Policy":
            {
                "Scheduler": "m3::RRScheduler",
                "OnACK": "m3::DumbTCPCallback"
            }
        }
        ```

    * Restrictions
        
        + You MUST NOT use `null` and empty objects as field value.
        + [Custom Fields](#customFields) of type object MUST contain a string subfield named `fieldClassName`, which value is the C++ class name of that field. 
        + If objects are contained by an array, all objects in such array MUST have same data structure. More precisely, correspond to same C++ class.

- Reserved Words & Other Elements

    We are accepting redundant fields in `m3` config files. Redundant fields are JSON fields that will not be parsed by m3 programs.  
    
    Comments is a very useful feature not supported by the JSON standard, so we recommand users to use a trick to support it:
    
    ```json
    {
        "m3-portLen-comment": "Max number of ports used by m3 Proxy.",
        "m3-portLen": 4096
    }
    ```

     To avoid name conflicts, we define all field names begin with `m3-` as reserved words, developer MUST NOT use these names for redundant fields in their config files to avoid unpredicted results. We gurantee that all other field names will never be used by us so feel free to leverage them for your own.
    
- <a name="customFields">Custom Fields</a>

    Developer can use custom fields to configure their own plugin/modules. To avoid potential naming issues, developer SHOULD use `m3-pakname.fieldname` to name their fields, where `pakname` is a standard [Java package name](https://docs.oracle.com/javase/tutorial/java/package/namingpkgs.html).

    Custom fields MUST be defined to be either _critical_ or _optional_. To solve the problem that some critical fields depends on some optional fields, for example, critical field `m3-car.wheelcount` depends on optional field `m3-car.enable`, define a optional field `m3-car`, and define its member `wheelcount` as critical.
    
## Packet Filter
The packet filter offered in `m3` (PacketFilter.cc/.hh) is based on GNU FLEX/BISON toolchain to parse the filter and generate grammar tree. This filter implemented a subset of BPF with restrictions shown below:  
- Only support TCP/UDP protocol qualifier
- Only support host, net and port(range) qualifier
- Do not support `and` parameter list e.g. `tcp port A and B`, please use `tcp port A or tcp port B` or `tcp port A, B` instead
- Simply travel the grammar tree when filtering, the performance needs further tests