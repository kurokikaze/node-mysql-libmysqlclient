/*
Copyright by node-mysql-libmysqlclient contributors.
See contributors list in README

See license text in LICENSE file
*/

#include "./mysql_bindings_connection.h"
#include "./mysql_bindings_statement.h"

Persistent<FunctionTemplate> MysqlConn::MysqlStatement::constructor_template;

void MysqlConn::MysqlStatement::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    // Constructor
    constructor_template = Persistent<FunctionTemplate>::New(t);
    constructor_template->Inherit(EventEmitter::constructor_template);
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template->SetClassName(String::NewSymbol("MysqlStatement"));

    // Methods
    ADD_PROTOTYPE_METHOD(statement, affectedRowsSync, AffectedRowsSync);
    ADD_PROTOTYPE_METHOD(statement, closeSync, CloseSync);
    ADD_PROTOTYPE_METHOD(statement, errnoSync, ErrnoSync);
    ADD_PROTOTYPE_METHOD(statement, errorSync, ErrorSync);
    ADD_PROTOTYPE_METHOD(statement, executeSync, ExecuteSync);
    ADD_PROTOTYPE_METHOD(statement, prepareSync, PrepareSync);
    ADD_PROTOTYPE_METHOD(statement, resetSync, ResetSync);
    ADD_PROTOTYPE_METHOD(statement, sqlStateSync, SqlStateSync);
}

MysqlConn::MysqlStatement::MysqlStatement(): EventEmitter() {
    _stmt = NULL;
}

MysqlConn::MysqlStatement::~MysqlStatement() {
    if (_stmt) {
        mysql_stmt_close(_stmt);
    }
}

Handle<Value> MysqlConn::MysqlStatement::New(const Arguments& args) {
    HandleScope scope;

    REQ_EXT_ARG(0, js_stmt);
    MYSQL_STMT *stmt = static_cast<MYSQL_STMT*>(js_stmt->Value());
    MysqlStatement *my_stmt = new MysqlStatement(stmt);
    my_stmt->Wrap(args.This());

    return args.This();
}

Handle<Value> MysqlConn::MysqlStatement::AffectedRowsSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt->_stmt);

    if (affected_rows == ((my_ulonglong)-1)) {
        return THREXC("Error occured in mysql_stmt_affected_rows(), -1 returned");
    }

    Local<Value> js_result = Integer::New(affected_rows);

    return scope.Close(Integer::New(affected_rows));
}

Handle<Value> MysqlConn::MysqlStatement::CloseSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Already closed");
    }

    if (mysql_stmt_close(stmt->_stmt)) {
        return THREXC("Error in mysql_stmt_close");
    }

    stmt->_stmt = NULL;

    return scope.Close(True());
}

Handle<Value> MysqlConn::MysqlStatement::ErrnoSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    uint32_t errno = mysql_stmt_errno(stmt->_stmt);

    Local<Value> js_result = Integer::New(errno);

    return scope.Close(js_result);
}

Handle<Value> MysqlConn::MysqlStatement::ErrorSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    const char *error = mysql_stmt_error(stmt->_stmt);

    Local<Value> js_result = String::New(error);

    return scope.Close(js_result);
}

Handle<Value> MysqlConn::MysqlStatement::ExecuteSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    if (mysql_stmt_execute(stmt->_stmt)) {
        return scope.Close(False());
    }

    return scope.Close(True());
}

Handle<Value> MysqlConn::MysqlStatement::PrepareSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (args.Length() == 0 || !args[0]->IsString()) {
        return THREXC("First arg of stmt.prepareSync() must be a string");
    }

    String::Utf8Value query(args[0]);

    int query_len = args[0]->ToString()->Utf8Length();

    if (mysql_stmt_prepare(stmt->_stmt, *query, query_len)) {
        return scope.Close(False());
    }

    return scope.Close(True());
}

Handle<Value> MysqlConn::MysqlStatement::ResetSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    if (mysql_stmt_reset(stmt->_stmt)) {
        return THREXC("Error in mysql_stmt_reset");
    }

    return scope.Close(True());
}

Handle<Value> MysqlConn::MysqlStatement::SqlStateSync(const Arguments& args) {
    HandleScope scope;

    MysqlStatement *stmt = OBJUNWRAP<MysqlStatement>(args.This());

    if (!stmt->_stmt) {
        return THREXC("Statement not initialized");
    }

    Local<Value> js_result = String::New(mysql_stmt_sqlstate(stmt->_stmt));

    return scope.Close(js_result);
}

