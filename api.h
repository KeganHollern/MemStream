//
// Created by Kegan Hollern on 12/24/23.
//

#ifndef MEMSTREAM_API_H
#define MEMSTREAM_API_H

#if defined(_WIN32)
#if defined(MEMSTREAM_EXPORTS)
#define MEMSTREAM_API __declspec(dllexport)
#else
#define MEMSTREAM_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define MEMSTREAM_API __attribute__ ((visibility ("default")))
#else
#define MEMSTREAM_API
#endif
#endif


#endif //MEMSTREAM_API_H
