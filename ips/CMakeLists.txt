# ips library set (mirrors ips/BUILD).

z2edit_library(ips ips.cc ips.h)
target_link_libraries(z2edit_ips PUBLIC z2edit::status)