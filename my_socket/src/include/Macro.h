#pragma once

#define DISALLOW_COPY(TypeName)                                \
  TypeName(const TypeName &) = delete;            /* NOLINT */ \
  TypeName &operator=(const TypeName &) = delete; /* NOLINT */

#define DISALLOW_MOVE(TypeName)                           \
  TypeName(TypeName &&) = delete;            /* NOLINT */ \
  TypeName &operator=(TypeName &&) = delete; /* NOLINT */

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName)                  \
  DISALLOW_MOVE(TypeName)
