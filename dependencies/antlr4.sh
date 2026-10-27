#!/usr/bin/env bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64/
export JRE_HOME=/usr/lib/jvm/java-17-openjdk-amd64/
pushd ../submodules/yaucl/submodules/antlr4
mvn clean install
popd
java -jar submodules/yaucl/submodules/antlr4/tool/target/antlr4-4.13.2-complete.jar -Dlanguage=Cpp -visitor $1 #-o antlr4/cpp