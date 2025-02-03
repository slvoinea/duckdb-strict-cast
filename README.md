# `try_cast_strict`

This extension allows you to perform a strict conversion when casting a string 
to a target type. It can convert to any type DuckDB recognizes, yet it is particularly
useful for converting to `INTEGER` and `DECIMAL`. In contrast to the DuckDB `try_cast`
implementation, it does not round numbers, but produces a `NULL` value instead.

`try_cast_strict` takes two positional arguments:
 - 1: the source to convert from, similar to `try_cast`. The source can be,
for instance, the name of the column to convert from (e.g., `my_column`) or a string
representation for a value (e.g., `"1.2"`); 
 - 2: a string representation for the type to convert to (e.g., `"INTEGER"`).
    > **NOTE**: This is different from the `try_cast` implementation, where the name of the 
    target type is a literal identifier as well.

**Examples**:

```sql
SELECT try_cast('1.12' AS DECIMAL(2,1)) AS result;
┌──────────────┐
│    result    │
│ decimal(2,1) │
├──────────────┤
│          1.1 │
└──────────────┘

SELECT try_cast_strict('1.12', 'DECIMAL(2,1)') AS result;
┌──────────────┐
│    result    │
│ decimal(2,1) │
├──────────────┤
│              │
└──────────────┘

SELECT try_cast_strict('1.1', 'DECIMAL(2,1)') AS result;
┌──────────────┐
│    result    │
│ decimal(2,1) │
├──────────────┤
│         1.1  │
└──────────────┘

SELECT try_cast('1.1' AS INTEGER) AS result;
┌────────┐
│ result │
│ int32  │
├────────┤
│      1 │
└────────┘

SELECT try_cast_strict('1.1', 'INTEGER') AS result;
┌────────┐
│ result │
│ int32  │
├────────┤
│        │
└────────┘

select try_cast_strict('1.11111119', 'FLOAT') as result;
┌───────────┐
│  result   │
│   float   │
├───────────┤
│ 1.1111112 │
└───────────┘
```

A three argument version is also available (`try_cast_strict_sp`), allowing to 
specify the decimal separator as the third argument.

**Example**:

```sql
SELECT try_cast_strict_sp('0,12e1', 'DECIMAL(3,1)', ',') AS result;
┌──────────────┐
│    result    │
│ decimal(3,1) │
├──────────────┤
│          1.2 │
└──────────────┘
```

## Limitations

- Handles exponent notation only when casting to `DECIMAL` (i.e., not when casting
to `INTEGER`). 



## Building
### Managing dependencies
DuckDB extensions uses VCPKG for dependency management. Enabling VCPKG is very 
simple: follow the [installation instructions](https://vcpkg.io/en/getting-started) or just run the following:
```shell
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_TOOLCHAIN_PATH=`pwd`/vcpkg/scripts/buildsystems/vcpkg.cmake
```
Note: VCPKG is only required for extensions that want to rely on it for dependency
management. If you want to develop an extension without dependencies, or want to do
your own dependency management, just skip this step. Note that the example extension
uses VCPKG to build with a dependency for instructive purposes, so when skipping 
this step the build may not work without removing the dependency.

### Build steps
Now to build the extension, run:
```sh
make
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/try_cast_strict/try_cast_strict.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically
loaded.
- `unittest` is the test runner of duckdb. Again, the extension is already linked 
into the binary.
- `try_cast_strict.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. The template
contains a single scalar function `try_cast_strict()` that takes a string arguments
and returns a string:
```
D select try_cast_strict('1', 'INTEGER') as result;
┌───────────────┐
│    result     │
│    varchar    │
├───────────────┤
│             1 │ 
└───────────────┘
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing 
DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can 
be run using:
```sh
make test
```

### Installing the deployed binaries
To install your extension binaries from S3, you will need to do two things. 
Firstly, DuckDB should be launched with the `allow_unsigned_extensions` option 
set to true. How to set this will depend on the client you're using. Some examples:

CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url
of your bucket + version of the extension you want to install. To do this run 
the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/<your_extension_name>/latest';
```
Note that the `/latest` path will allow you to install the latest extension 
version available for your current version of DuckDB. To specify a specific 
version, you can pass the version instead.

After running these steps, you can install and load your extension using the
regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL try_cast_strict
LOAD try_cast_strict
```
---

This repository is based on https://github.com/duckdb/extension-template, check it 
out if you want to build and ship your own DuckDB extension.