Hybrid Authentication (OpenId + OAuth) for Google Data API
==========================================================
 
References:
-----------
  http://code.google.com/apis/accounts/docs/OpenID.html
  http://step2.googlecode.com/svn/spec/openid_oauth_extension/latest/openid_oauth_extension.html
 
About Domains/Realm
-------------------
  If you've been using both OpenID and OAuth separately, you may be able to use the same domain
registration as long as the OpenID realm parameter value matches the OAuth consumer  parameter value. 
If the values don't match, either re-register a domain consistent with the OpenID realm value, 
or change the value of realm. However, changing the realm may not be a viable option if you have 
existing users with OpenID.
Note from Google, http://code.google.com/apis/accounts/docs/OpenID.html

About Google OpenID
-------------------
  The Google-supplied identifier, which has no connection to the user's actual Google account name or password, 
is a persistent value; it remains constant even if the user changes their Google user name and/or email address.
This identifier is also a "directed identity", that is, Google returns a different value to each relying party. 
Google uses the request parameter openid.realm to recognize the relying party, so if the third-party application 
decides to change this value, all user identifiers will change.
Note from Google, http://code.google.com/apis/accounts/docs/OpenID.html

Summary: 
1. You must use the same domain name for A) Consumer key B) OpenID realm C) OpenId return path
2. Google OpenID identifiers are domain dependant.
3Thus: You cannot move resulting user accounts (with OpenIDs) to a different domain.