#include <daemons.h>
#include <gmcp_defines.h>

inherit __DIR__ "gmcp_module";

void Login(string submodule, mapping data) {
  object prev = previous_object();

  switch(submodule) {
    case "Credentials" : {
      string username, account_name, password, character, curr, test;
      mapping account;

      username = lower_case(data["account"]);
      sscanf(username, "%s@%s", character, account_name);

      account = ACCOUNT_D->loadAccount(account_name);
      password = data["password"];

      if(!account || !account["password"]) {
        GMCP_D->send_gmcp(prev,
          GMCP_PKG_CHAR_LOGIN_RESULT,
          ([ "success" : "false", "message": "Invalid account name."])
        );

        tell(prev, "Invalid account name.\n");
        prev->remove();
        return;
      }

      curr = account["password"];
      test = crypt(password, curr);
      if(test != curr) {
        GMCP_D->send_gmcp(prev,
          GMCP_PKG_CHAR_LOGIN_RESULT,
          ([ "success" : "false", "message": "Invalid password."])
        );
        tell(prev, "Invalid password.\n");
        prev->remove();
        return;
      }

      GMCP_D->send_gmcp(prev,
        GMCP_PKG_CHAR_LOGIN_RESULT,
        ([ "success" : "true", "message": "Login successful."])
      );

      prev->gmcp_authenticated(username, character);
    }
  }
}

void Items(string submodule, string target) {
  object prev = previous_object();

  switch(submodule) {
    case "Contents" :
      GMCP_D->send_gmcp(prev, GMCP_PKG_CHAR_ITEMS_LIST, target);
      break;
    case "Inv" :
      GMCP_D->send_gmcp(prev, GMCP_PKG_CHAR_ITEMS_LIST, GMCP_LIST_INV);
      break;
    case "Room" :
      GMCP_D->send_gmcp(prev, GMCP_PKG_CHAR_ITEMS_LIST, GMCP_LIST_ROOM);
      break;
  }
}
