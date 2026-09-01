# Internal OpenAPI readme

## Introduction

The OpenAPI module is very different from the typical Qt module. This document
tries to record and explain some decisions that were taken during development.

## Roots of the module

The module is based on the
[OpenAPI Generator project](https://github.com/OpenAPITools/openapi-generator),
and specifically on its Qt5-based generator, which was contributed to the
upstream by the community.

As a result, we need to reuse the upstream Apache 2.0 license for the Java
generator plugin and the template files.

## Content of the module

The module consists of the following parts:

* Java Qt6 generator plugin - the plugin for the upstream OpenAPI generator.
It is derived from the Qt5-based upstream plugin.
* Templates to generate the C++ code ( `.mustache` files). These are also
derived from the Qt5-based upstream plugin. However, we added a lot of fixes
and improvements to them.
* CMake code that provides a user API to generate the client libraries
(`qt_add_openapi_client()`). That's fully implemented by us.
* Tests and examples. Also fully implemented by us.

## Design decisions

The original Qt5-based plugin took a YAML file and generated one large library
with all the necessary code. That library included both user-facing API classes
and some internal implementation (parts of which might be also used in the user
code).

If the user wanted to generate two separate libraries from two different YAML
files, then a significant part of the code would be duplicated. This
potentially leads to linking issues and ODR violations, unless the user
generates two libraries in different namespaces.

We decided that this approach is inconvenient and leads to a lot of code
duplication. As a result, we moved the shared code into a common library. The
template files for the common library are located in the `common` subdirectory
of the templates.

This common library is shipped as a part of the Qt Project, and can be
referred to as `Qt::OpenApiCommon` from the CMake code.

At the same time, we also wanted to keep the possibility to use our plugin
directly from the command line, like all other plugins of the upstream
generator. That makes the common library optional - user can build their own
version of the library.

Such approach imposes certain restrictions on the templates for the common
library. More on that later.

### CMake function details

When the user invokes the `qt_add_openapi_client()` function, only the client
code is generated and linked as a library. This library also implicitly links
to `Qt::OpenApiCommon`.

However, the CMake function also has an internal mode that is used to build
the common library. We use it when building Qt.

### Rules for the common library code

As noted above, we ship the common library as a part of the Qt Project. This
means that **once the module is out of Tech Preview stage**, we will need to
provide source and binary compatibility for the classes exported from the
common library.

This implies the usual coding guidelines that we use in other modules for the
public API.

However, we also want to keep the possibility of building the code of the
common library directly from the command line, thus ignoring the common library
that is provided by the Qt Project. This means that we cannot really use
private APIs and private Qt macros (those starting with `QT_`) in the common
library code.

### Rules for the client library code

Client library is supposed to be generated at the user side, so we do not need
to care about binary compatibility, but source compatibility should still be
preserved as much as possible, because we do not want to force the user to
change their code every time the new version of Qt is released.

## Configuration with CMake

We decided to generate the client code at the configuration time, not at build
time. This also means that we need to build the Qt6 Java Generator plugin at
configuration time.

The reason for such approach is that we cannot know the names of all generated
files in advance (because they depend on the content of the provided YAML
file). As a result, if we run the generator at build time, we cannot reliably
specify the SOURCES of the client library.

Our first approach was to use one common file that includes all generated
headers and sources. This file had predefined name, so we could set up the
dependencies properly. This approach, however, had several drawbacks:

* Bad debugging experience: when jumping through the includes, the user ends
up in an individual source/header file. But adding a breakpoint there does not
work, because the compiler actually used the large file that has everything.
* Bad IDE support: you do not get a full list of generated files in the user
project.
* The compiler (or moc) potentially could have issues with processing such a
large file.

The approach with generation at configuration time solves all these problems,
because it allows us to extract the list of generated files, and properly set
up dependencies for the client projects. However, it results in longer
configuration times + a more complicated CMake code, because we now need to
manually track some dependencies to figure out when we need to re-run the
generation, and when we can skip it during the configuration step.
