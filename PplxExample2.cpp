/***
* ==++==
*
* Copyright (c) Microsoft Corporation. All rights reserved. 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* searchfile.cpp - Simple cmd line application that uses a variety of streams features to search a file,
*      store the results, and write results back to a new file.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
// Modified by John R. Bandela to use pplx_helper
#include "pplx_helper.hpp"
#include <filestream.h>
#include <containerstream.h>
#include <producerconsumerstream.h>

using namespace utility;
using namespace concurrency::streams;



#ifdef _MS_WINDOWS
int wmain(int argc, wchar_t *args[])
#else
int main(int argc, char *args[])
#endif
{
    if(argc != 4)
    {
        printf("Usage: PplxExample2 input_file search_string output_file\n");
        return -1;
    }
    const string_t inFileName = args[1];
    const std::string searchString = utility::conversions::to_utf8string(args[2]);
    const string_t outFileName = args[3];
    producer_consumer_buffer<char> lineResultsBuffer;
	
	// Find all matches in file.
	basic_ostream<char> outLineResults(lineResultsBuffer);

    auto reader = pplx_helper::do_async([&](pplx_helper::async_helper<void> helper){
        auto inFile = helper.await(file_stream<char>::open_istream(inFileName));
        int lineNumber = 1;
        bool done = false;
        while(!done){
            container_buffer<std::string> inLine;
            auto bytesRead = helper.await(inFile.read_line(inLine));
            if(bytesRead==0 && inFile.is_eof()){
                done = true;
            }
            else if(inLine.collection().find(searchString) != std::string::npos){
                helper.await(outLineResults.print("line "));
                helper.await(outLineResults.print(lineNumber++));
                helper.await(outLineResults.print(":"));
                container_buffer<std::string> outLine(std::move(inLine.collection()));
                helper.await(outLineResults.write(outLine,outLine.collection().size()));
                helper.await(outLineResults.print("\r\n"));
            }
            else{
                ++lineNumber;
            }

        }
        helper.await(inFile.close() && outLineResults.close());
    });

    auto writer = pplx_helper::do_async([&](pplx_helper::async_helper<void> helper){
        basic_istream<char> inLineResults(lineResultsBuffer);
        auto outFile = helper.await(file_stream<char>::open_ostream(outFileName,std::ios::trunc));
        auto currentIndex = 0;
        bool done = false;
        while(!done){
            container_buffer<std::string> lineData;
            auto bytesRead = helper.await(inLineResults.read_line(lineData));
            if(bytesRead==0 && inLineResults.is_eof()){
                done = true;
            }
            else{
                container_buffer<std::string> lineDataOut(std::move(lineData.collection()));
                helper.await(outFile.write(lineDataOut,lineDataOut.collection().size()));
                helper.await(outFile.print("\r\n"));
            }
        }
        helper.await(inLineResults.close() && outFile.close());

    });


    try{
	// Wait for everything to complete and catch any exceptions
	(reader && writer).wait();

    }
    catch(std::exception& e){
        std::cerr << e.what();
    }

    return 0;
}