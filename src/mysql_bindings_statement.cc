/*!
 * Copyright by Oleg Efimov and node-mysql-libmysqlclient contributors
 * See contributors list in README
 *
 * See license text in LICENSE file
 */

/**
 * Include headers
 *
 * @ignore
 */
#include "./mysql_bindings_connection.h"
#include "./mysql_bindings_result.h"
#include "./mysql_bindings_statement.h"

/**
 * Init V8 structures for MysqlStatement class
 *
 * @ignore
 */
Persistent<FunctionTemplate> MysqlStatement::constructor_template;

void MysqlStatement::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    // Constructor
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("MysqlStatement"));

    Local<ObjectTemplate> instance_template =
        constructor_template->InstanceTemplate();

    // Constants
    NODE_DEFINE_CONSTANT(instance_template, STMT_ATTR_UPDATE_MAX_LENGTH);
    NODE_DEFINE_CONSTANT(instance_template, STMT_ATTR_CURSOR_TYPE);
    NODE_DEFINE_CONSTANT(instance_template, STMT_ATTR_PREFETCH_ROWS);

    // Properties
    instance_template->SetAccessor(String::New("paramCount"), ParamCountGetter);

    // Methods
    ADD_PROTOTYPE_METHOD(statement, affectedRowsSync, AffectedRowsSync);
    ADD_PROTOTYPE_METHOD(statement, attrGetSync, AttrGetSync);
    ADD_PROTOTYPE_METHOD(statement, attrSetSync, AttrSetSync);
    ADD_PROTOTYPE_METHOD(statement, bindParamsSync, BindParamsSync);
    ADD_PROTOTYPE_METHOD(statement, closeSync, CloseSync);
    ADD_PROTOTYPE_METHOD(statement, dataSeekSync, DataSeekSync);
    ADD_PROTOTYPE_METHOD(statement, errnoSync, ErrnoSync);
    ADD_PROTOTYPE_METHOD(statement, errorSync, ErrorSync);
    ADD_PROTOTYPE_METHOD(statement, executeSync, ExecuteSync);
    ADD_PROTOTYPE_METHOD(statement, fieldCountSync, FieldCountSync);
    ADD_PROTOTYPE_METHOD(statement, freeResultSync, FreeResultSync);
    ADD_PROTOTYPE_METHOD(statement, lastInsertIdSync, LastInsertIdSync);
    ADD_PROTOTYPE_METHOD(statement, numRowsSync, NumRowsSync);
    ADD_PROTOTYPE_METHOD(statement, prepareSync, PrepareSync);
    ADD_PROTOTYPE_METHOD(statement, resetSync, ResetSync);
    ADD_PROTOTYPE_METHOD(statement, resultMetadataSync, ResultMetadataSync);
    ADD_PROTOTYPE_METHOD(statement, storeResultSync, StoreResultSync);
    ADD_PROTOTYPE_METHOD(statement, sqlStateSync, SqlStateSync);

    // Make it visible in JavaScript
    target->Set(String::NewSymbol("MysqlStatement"),
                                  constructor_template->GetFunction());
}

MysqlStatement::MysqlStatement(MYSQL_STMT *my_stmt): EventEmitter() {
    this->_stmt = my_stmt;
    this->binds = NULL;
    this->param_count = 0;
    this->prepared = false;
    this->stored = false;
}

MysqlStatement::~MysqlStatement() {
    if (this->_stmt) {
        if (this->prepared) {
            for (uint64_t i = 0; i < this->param_count; i++) {
                if (this->binds[i].buffer_type == MYSQL_TYPE_LONG) {
                    if (this->binds[i].is_unsigned) {
                        delete static_cast<unsigned int *>(this->binds[i].buffer); // NOLINT
                    } else {
                        delete static_cast<int *>(this->binds[i].buffer);
                    }
                } else if (this->binds[i].buffer_type == MYSQL_TYPE_DOUBLE) {
                    delete static_cast<double *>(this->binds[i].buffer);
                } else if (this->binds[i].buffer_type == MYSQL_TYPE_STRING) {
                    // TODO(Sannis): Or delete?
                    delete[] static_cast<char *>(this->binds[i].buffer);
                    delete static_cast<unsigned long *>(this->binds[i].length); // NOLINT
                } else if (this->binds[i].buffer_type == MYSQL_TYPE_DATETIME) {
                    delete static_cast<MYSQL_TIME *>(this->binds[i].buffer);
                } else {
                    printf("MysqlStatement::~MysqlStatement: o_0\n");
                }
            }
            delete[] this->binds;
        }
        mysql_stmt_free_result(this->_stmt);
        mysql_stmt_close(this->_stmt);
    }
}

/**
 * Create new MySQL statement object
 *
 * @constructor
 */
Handle<Value> MysqlStatement::New(const Arguments& args) {
    HandleScope scope;

    REQ_EXT_ARG(0, js_stmt);
    MYSQL_STMT *my_stmt = static_cast<MYSQL_STMT*>(js_stmt->Value());
    MysqlStatement *binding_stmt = new MysqlStatement(my_stmt);
    binding_stmt->Wrap(args.This());

    return args.This();
}

Handle<Value> MysqlStatement::ParamCountGetter(Local<String> property,
                                                          const AccessorInfo &info) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(info.Holder());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    return scope.Close(Integer::New(stmt->param_count));
}

Handle<Value> MysqlStatement::AffectedRowsSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt->_stmt);

    if (affected_rows == ((my_ulonglong)-1)) {
        return THREXC("Error occured in mysql_stmt_affected_rows()");
    }

    Local<Value> js_result = Integer::New(affected_rows);

    return scope.Close(Integer::New(affected_rows));
}

Handle<Value> MysqlStatement::AttrGetSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    REQ_INT_ARG(0, attr_integer_key)
    enum_stmt_attr_type attr_key =
                        static_cast<enum_stmt_attr_type>(attr_integer_key);

    // TODO(Sannis): Possible error, see Integer::NewFromUnsigned, 32/64
    unsigned long attr_value; // NOLINT

    if (mysql_stmt_attr_get(stmt->_stmt, attr_key, &attr_value)) {
        return THREXC("This attribute isn't supported by libmysqlclient");
    }

    switch (attr_key) {
        case STMT_ATTR_UPDATE_MAX_LENGTH:
            return scope.Close(Boolean::New(attr_value));
            break;
        case STMT_ATTR_CURSOR_TYPE:
        case STMT_ATTR_PREFETCH_ROWS:
            return scope.Close(Integer::NewFromUnsigned(attr_value));
            break;
        default:
            return THREXC("This attribute isn't supported yet");
    }

    return THREXC("Control reaches end of non-void function :-D");
}

Handle<Value> MysqlStatement::AttrSetSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    REQ_INT_ARG(0, attr_integer_key)
    enum_stmt_attr_type attr_key =
                        static_cast<enum_stmt_attr_type>(attr_integer_key);
    int r = 1;

    switch (attr_key) {
        case STMT_ATTR_UPDATE_MAX_LENGTH:
            {
            REQ_BOOL_ARG(1, attr_bool_value);
            r = mysql_stmt_attr_set(stmt->_stmt, attr_key, &attr_bool_value);
            }
            break;
        case STMT_ATTR_CURSOR_TYPE:
        case STMT_ATTR_PREFETCH_ROWS:
            {
            REQ_UINT_ARG(1, attr_uint_value);
            r = mysql_stmt_attr_set(stmt->_stmt, attr_key, &attr_uint_value);
            }
            break;
        default:
            return THREXC("This attribute isn't supported yet");
    }

    if (r) {
        return THREXC("This attribute isn't supported by libmysqlclient");
    }

    return scope.Close(True());
}

Handle<Value> MysqlStatement::BindParamsSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    REQ_ARRAY_ARG(0, js_params);

    uint32_t i = 0;
    Local<Value> js_param;

    // For debug
    /*String::Utf8Value *str;
    for (i = 0; i < js_params->Length(); i++) {
        str = new String::Utf8Value(js_params->Get(Integer::New(i))->ToString());
        printf("%d: %s\n", i, **str);
    }*/

    if (js_params->Length() != stmt->param_count) {
        return THREXC("Array length doesn't match number of parameters in prepared statement"); // NOLINT
    }

    int *int_data;
    unsigned int *uint_data;
    double *double_data;
    unsigned long *str_length; // NOLINT
    char *str_data;
    time_t date_timet;
    struct tm date_timeinfo;
    MYSQL_TIME *date_data;

    for (i = 0; i < js_params->Length(); i++) {
        js_param = js_params->Get(i);

        if (js_param->IsUndefined()) {
            return THREXC("All arguments must be defined");
        }

        if (js_param->IsNull()) {
            int_data = new int;
            *int_data = 0;

            stmt->binds[i].buffer_type = MYSQL_TYPE_NULL;
            stmt->binds[i].buffer = int_data;
            // TODO(Sannis): Fix this error
            stmt->binds[i].is_null = 0;
        } else if (js_param->IsInt32()) {
            int_data = new int;
            *int_data = js_param->Int32Value();

            stmt->binds[i].buffer_type = MYSQL_TYPE_LONG;
            stmt->binds[i].buffer = int_data;
            stmt->binds[i].is_null = 0;
            stmt->binds[i].is_unsigned = false;
        } else if (js_param->IsUint32()) {
            uint_data = new unsigned int;
            *uint_data = js_param->Uint32Value();

            stmt->binds[i].buffer_type = MYSQL_TYPE_LONG;
            stmt->binds[i].buffer = uint_data;
            stmt->binds[i].is_null = 0;
            stmt->binds[i].is_unsigned = true;
        } else if (js_param->IsNumber()) {
            double_data = new double;
            *double_data = js_param->NumberValue();

            stmt->binds[i].buffer_type = MYSQL_TYPE_DOUBLE;
            stmt->binds[i].buffer = double_data;
            stmt->binds[i].is_null = 0;
        } else if (js_param->IsString()) {
            // TODO(Sannis): Simplify this if possible
            str_data = strdup(**(new String::Utf8Value(js_param->ToString())));
            str_length = new unsigned long; // NOLINT
            *str_length = js_param->ToString()->Length();

            stmt->binds[i].buffer_type = MYSQL_TYPE_STRING;
            stmt->binds[i].buffer =  str_data;
            stmt->binds[i].is_null = 0;
            stmt->binds[i].length = str_length;
        } else if (js_param->IsDate()) {
            date_data = new MYSQL_TIME;
            date_timet = static_cast<time_t>(js_param->NumberValue()/1000);
            if (!gmtime_r(&date_timet, &date_timeinfo)) {
                return THREXC("Error occured in gmtime_r()");
            }
            date_data->year = date_timeinfo.tm_year + 1900;
            date_data->month = date_timeinfo.tm_mon + 1;
            date_data->day = date_timeinfo.tm_mday;
            date_data->hour = date_timeinfo.tm_hour;
            date_data->minute = date_timeinfo.tm_min;
            date_data->second = date_timeinfo.tm_sec;

            stmt->binds[i].buffer_type = MYSQL_TYPE_DATETIME;
            stmt->binds[i].buffer = date_data;
            stmt->binds[i].is_null = 0;
        } else {
            return THREXC("o_0");
        }
    }

    if (mysql_stmt_bind_param(stmt->_stmt, stmt->binds)) {
      return THREXC("mysql_stmt_bind_param() failed");
    }

    return scope.Close(True());
}


Handle<Value> MysqlStatement::CloseSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    if (mysql_stmt_close(stmt->_stmt)) {
        return THREXC("Error in mysql_stmt_close");
    }

    stmt->_stmt = NULL;

    return scope.Close(True());
}

Handle<Value> MysqlStatement::DataSeekSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;
    MYSQLSTMT_MUSTBE_STORED;

    REQ_UINT_ARG(0, row_num)

    // TODO(Sannis): Can we implement this?
    /*if (mysql_stmt_is_unbuffered(stmt->_stmt)) {
        return THREXC("Function cannot be used before store all results");
    }*/

    if (row_num < 0 || row_num >= mysql_stmt_num_rows(stmt->_stmt)) {
        return THREXC("Invalid row offset");
    }

    mysql_stmt_data_seek(stmt->_stmt, row_num);

    return Undefined();
}

Handle<Value> MysqlStatement::ErrnoSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    uint32_t errno = mysql_stmt_errno(stmt->_stmt);

    Local<Value> js_result = Integer::New(errno);

    return scope.Close(js_result);
}

Handle<Value> MysqlStatement::ErrorSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    const char *error = mysql_stmt_error(stmt->_stmt);

    Local<Value> js_result = String::New(error);

    return scope.Close(js_result);
}

Handle<Value> MysqlStatement::ExecuteSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    if (mysql_stmt_execute(stmt->_stmt)) {
        return scope.Close(False());
    }

    return scope.Close(True());
}

Handle<Value> MysqlStatement::FieldCountSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    return scope.Close(Integer::New(mysql_stmt_field_count(stmt->_stmt)));
}

/**
 * Frees stored result memory for the given statement handle
 *
 * @return {Boolean}
 */
Handle<Value> MysqlStatement::FreeResultSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    return scope.Close(!mysql_stmt_free_result(stmt->_stmt) ? True() : False());
}

Handle<Value> MysqlStatement::LastInsertIdSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    Local<Value> js_result = Integer::New(mysql_stmt_insert_id(stmt->_stmt));

    return scope.Close(js_result);
}

Handle<Value> MysqlStatement::NumRowsSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;
    MYSQLSTMT_MUSTBE_STORED;  // TODO(Sannis): Or all result already fetched!

    // TODO(Sannis): Can we implement this?
    /*if (mysql_stmt_is_unbuffered(stmt->_stmt)) {
        return THREXC("Function cannot be used before store all results");
    }*/

    Local<Value> js_result = Integer::New(mysql_stmt_num_rows(stmt->_stmt));

    return scope.Close(js_result);
}

/**
 * Prepare statement by given query
 *
 * @param {String} query
 * @return {Boolean}
 */
Handle<Value> MysqlStatement::PrepareSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    REQ_STR_ARG(0, query)

    // TODO(Sannis): Smth else? close/reset
    stmt->prepared = false;

    if (mysql_stmt_prepare(stmt->_stmt, *query, query.length())) {
        return scope.Close(False());
    }

    if (stmt->binds) {
        delete[] stmt->binds;
    }

    stmt->param_count = mysql_stmt_param_count(stmt->_stmt);

    if (stmt->param_count > 0) {
        stmt->binds = new MYSQL_BIND[stmt->param_count];
        if (!stmt->binds) {
            V8::LowMemoryNotification();
            return THREXC("Could not allocate enough memory");
        }
        memset(stmt->binds, 0, stmt->param_count*sizeof(MYSQL_BIND));

        // TODO(Sannis): Smth else?
    }

    stmt->prepared = true;

    return scope.Close(True());
}

Handle<Value> MysqlStatement::ResetSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;


    if (mysql_stmt_reset(stmt->_stmt)) {
        return THREXC("Error in mysql_stmt_reset");
    }

    return scope.Close(True());
}

Handle<Value> MysqlStatement::ResultMetadataSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    MYSQL_RES *my_result = mysql_stmt_result_metadata(stmt->_stmt);

    if (!my_result) {
        return scope.Close(False());
    }

    int argc = 2;
    Local<Value> argv[2];
    argv[0] = External::New(my_result);
    argv[1] = Integer::New(mysql_stmt_field_count(stmt->_stmt));
    Persistent<Object> js_result(MysqlResult::constructor_template->
                                 GetFunction()->NewInstance(argc, argv));

    return scope.Close(js_result);
}

Handle<Value> MysqlStatement::StoreResultSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;
    MYSQLSTMT_MUSTBE_PREPARED;

    if (mysql_stmt_store_result(stmt->_stmt) != 0) {
        return scope.Close(False());
    }

    stmt->stored = true;

    return scope.Close(True());
}

Handle<Value> MysqlStatement::SqlStateSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    MYSQLSTMT_MUSTBE_INITIALIZED;

    Local<Value> js_result = String::New(mysql_stmt_sqlstate(stmt->_stmt));

    return scope.Close(js_result);
}

