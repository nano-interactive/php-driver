#include "php_cassandra.h"
#include "util/consistency.h"
#include "util/math.h"

zend_class_entry *cassandra_execution_options_ce = NULL;

ZEND_EXTERN_MODULE_GLOBALS(cassandra)

PHP_METHOD(ExecutionOptions, __construct)
{
  zval *options = NULL;
  cassandra_execution_options *self = NULL;
  php5to7_zval *consistency = NULL;
  php5to7_zval *serial_consistency = NULL;
  php5to7_zval *page_size = NULL;
  php5to7_zval *timeout = NULL;
  php5to7_zval *arguments = NULL;
  php5to7_zval *timestamp = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &options) == FAILURE) {
    return;
  }

  if (!options) return;

  if (Z_TYPE_P(options) != IS_ARRAY) {
    INVALID_ARGUMENT(options, "an array");
  }

  self = PHP_CASSANDRA_GET_EXECUTION_OPTIONS(getThis());

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "consistency", sizeof("consistency"), consistency)) {
    if (php_cassandra_get_consistency(PHP5TO7_ZVAL_MAYBE_DEREF(consistency), &self->consistency TSRMLS_CC) == FAILURE) {
      return;
    }
  }

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "serial_consistency", sizeof("serial_consistency"), serial_consistency)) {
    if (php_cassandra_get_serial_consistency(PHP5TO7_ZVAL_MAYBE_DEREF(serial_consistency), &self->serial_consistency TSRMLS_CC) == FAILURE) {
      return;
    }
  }

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "page_size", sizeof("page_size"), page_size)) {
    if (Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(page_size)) != IS_LONG || Z_LVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(page_size)) <= 0) {
      throw_invalid_argument(PHP5TO7_ZVAL_MAYBE_DEREF(page_size), "page_size", "greater than zero" TSRMLS_CC);
      return;
    }
    self->page_size = Z_LVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(page_size));
  }

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "timeout", sizeof("timeout"), timeout)) {
    if (!(Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(timeout)) == IS_LONG   && Z_LVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(timeout)) > 0) &&
        !(Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(timeout)) == IS_DOUBLE && Z_DVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(timeout)) > 0) &&
        !(Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(timeout)) == IS_NULL)) {
      throw_invalid_argument(PHP5TO7_ZVAL_MAYBE_DEREF(timeout), "timeout", "a number of seconds greater than zero or null" TSRMLS_CC);
      return;
    }

    PHP5TO7_ZVAL_COPY(PHP5TO7_ZVAL_MAYBE_P(self->timeout), PHP5TO7_ZVAL_MAYBE_DEREF(timeout));
  }

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "arguments", sizeof("arguments"), arguments)) {
    if (Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(arguments)) != IS_ARRAY) {
      throw_invalid_argument(PHP5TO7_ZVAL_MAYBE_DEREF(arguments), "arguments", "an array" TSRMLS_CC);
      return;
    }
    PHP5TO7_ZVAL_COPY(PHP5TO7_ZVAL_MAYBE_P(self->arguments), PHP5TO7_ZVAL_MAYBE_DEREF(arguments));
  }

  if (PHP5TO7_ZEND_HASH_FIND(Z_ARRVAL_P(options), "timestamp", sizeof("timestamp"), timestamp)) {
    if (Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp)) == IS_LONG) {
      self->timestamp = Z_LVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp));
    } else if (Z_TYPE_P(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp)) == IS_STRING) {
      if (!php_cassandra_parse_bigint(Z_STRVAL_P(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp)),
                                      Z_STRLEN_P(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp)),
                                      &self->timestamp TSRMLS_CC)) {
        return;
      }
    } else {
      throw_invalid_argument(PHP5TO7_ZVAL_MAYBE_DEREF(timestamp), "timestamp", "an integer or integer string" TSRMLS_CC);
      return;
    }
  }
}

PHP_METHOD(ExecutionOptions, __get)
{
  char *name;
  php5to7_size name_len;

  cassandra_execution_options *self = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
    return;
  }

  self = PHP_CASSANDRA_GET_EXECUTION_OPTIONS(getThis());

  if (name_len == 11 && strncmp("consistency", name, name_len) == 0) {
    if (self->consistency == -1) {
      RETURN_NULL();
    }
    RETURN_LONG(self->consistency);
  } else if (name_len == 17 && strncmp("serialConsistency", name, name_len) == 0) {
    if (self->serial_consistency == -1) {
      RETURN_NULL();
    }
    RETURN_LONG(self->serial_consistency);
  } else if (name_len == 8 && strncmp("pageSize", name, name_len) == 0) {
    if (self->page_size == -1) {
      RETURN_NULL();
    }
    RETURN_LONG(self->page_size);
  } else if (name_len == 7 && strncmp("timeout", name, name_len) == 0) {
    if (PHP5TO7_ZVAL_IS_UNDEF(self->timeout)) {
      RETURN_NULL();
    }
    RETURN_ZVAL(PHP5TO7_ZVAL_MAYBE_P(self->timeout), 1, 0);
  } else if (name_len == 9 && strncmp("arguments", name, name_len) == 0) {
    if (PHP5TO7_ZVAL_IS_UNDEF(self->arguments)) {
      RETURN_NULL();
    }
    RETURN_ZVAL(PHP5TO7_ZVAL_MAYBE_P(self->arguments), 1, 0);
  } else if (name_len == 9 && strncmp("timestamp", name, name_len) == 0) {
    char *string;
    if (self->timestamp == INT64_MIN) {
      RETURN_NULL();
    }
#ifdef WIN32
    spprintf(&string, 0, "%I64d", (long long int) self->timestamp);
#else
    spprintf(&string, 0, "%lld", (long long int) self->timestamp);
#endif
    PHP5TO7_RETVAL_STRING(string);
    efree(string);
  }
}

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 0)
  ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo___get, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static zend_function_entry cassandra_execution_options_methods[] = {
  PHP_ME(ExecutionOptions, __construct, arginfo__construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
  PHP_ME(ExecutionOptions, __get, arginfo___get, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers cassandra_execution_options_handlers;

static HashTable *
php_cassandra_execution_options_properties(zval *object TSRMLS_DC)
{
  HashTable *props = zend_std_get_properties(object TSRMLS_CC);

  return props;
}

static int
php_cassandra_execution_options_compare(zval *obj1, zval *obj2 TSRMLS_DC)
{
  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  return Z_OBJ_HANDLE_P(obj1) != Z_OBJ_HANDLE_P(obj1);
}

static void
php_cassandra_execution_options_free(php5to7_zend_object_free *object TSRMLS_DC)
{
  cassandra_execution_options *self =
      PHP5TO7_ZEND_OBJECT_GET(execution_options, object);

  PHP5TO7_ZVAL_MAYBE_DESTROY(self->arguments);
  PHP5TO7_ZVAL_MAYBE_DESTROY(self->timeout);

  zend_object_std_dtor(&self->zval TSRMLS_CC);
  PHP5TO7_MAYBE_EFREE(self);
}

static php5to7_zend_object
php_cassandra_execution_options_new(zend_class_entry *ce TSRMLS_DC)
{
  cassandra_execution_options *self =
      PHP5TO7_ZEND_OBJECT_ECALLOC(execution_options, ce);

  self->consistency = -1;
  self->serial_consistency = -1;
  self->page_size = -1;
  self->timestamp = INT64_MIN;
  PHP5TO7_ZVAL_UNDEF(self->arguments);
  PHP5TO7_ZVAL_UNDEF(self->timeout);

  PHP5TO7_ZEND_OBJECT_INIT(execution_options, self, ce);

}

void cassandra_define_ExecutionOptions(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "Cassandra\\ExecutionOptions", cassandra_execution_options_methods);
  cassandra_execution_options_ce = zend_register_internal_class(&ce TSRMLS_CC);
  cassandra_execution_options_ce->ce_flags     |= PHP5TO7_ZEND_ACC_FINAL;
  cassandra_execution_options_ce->create_object = php_cassandra_execution_options_new;

  memcpy(&cassandra_execution_options_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  cassandra_execution_options_handlers.get_properties  = php_cassandra_execution_options_properties;
  cassandra_execution_options_handlers.compare_objects = php_cassandra_execution_options_compare;
}
