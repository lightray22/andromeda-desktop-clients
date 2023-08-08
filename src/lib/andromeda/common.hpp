#ifndef LIBA2_COMMON_H_
#define LIBA2_COMMON_H_

#define DELETE_COPY(cl) cl(const cl&) = delete; cl& operator=(const cl&) = delete;
#define DELETE_MOVE(cl) cl(cl&&) = delete; cl& operator=(cl&&) = delete;

#endif // LIBA2_COMMON_H_
