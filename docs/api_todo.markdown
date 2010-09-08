TODO: NodeJS MySQL/libmysqlclient bindings API
==============================================

Options
-------

    bool options (int $option , mixed $value);
    void set_local_infile_default (void);
    bool set_local_infile_handler (callback $read_func);

I think that this functions not so important at this moment.
We can implement MysqlSyncConn::setOption in first stable release,
and MysqlSyncConn::setLocalInfile{Default,Handler} later.

Statement class
---------------

[+] mysql_stmt_affected_rows()
[+] mysql_stmt_attr_get()
[+] mysql_stmt_attr_set()
mysql_stmt_bind_param()
mysql_stmt_bind_result()
[+] mysql_stmt_close()
[+] mysql_stmt_data_seek()
[+] mysql_stmt_errno()
[+] mysql_stmt_error()
[+] mysql_stmt_execute()
mysql_stmt_fetch()
[-] mysql_stmt_fetch_column() (needed?)
[+] mysql_stmt_field_count()
[+] mysql_stmt_free_result()
[+] mysql_stmt_init()
[+] mysql_stmt_insert_id()
[+] mysql_stmt_num_rows()
[+] mysql_stmt_param_count()
[-] mysql_stmt_param_metadata() (because this function currently does nothing)
[?] mysql_stmt_prepare() (check and rewrite conn.prepareSync/stmt.New/stmt.prepareSync)
[+] mysql_stmt_reset()
[+] mysql_stmt_result_metadata()
[-] mysql_stmt_row_seek() (needed?)
[-] mysql_stmt_row_tell() (needed?)
mysql_stmt_send_long_data()
[+] [simple test] mysql_stmt_sqlstate()
[+] mysql_stmt_store_result()

