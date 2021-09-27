
#ifndef LIBA2_ADOPTED_H_
#define LIBA2_ADOPTED_H_

#include "PlainFolder.hpp"
#include "Utilities.hpp"

class Backend;
class Folder;

/** Lists items owned but residing in other users' folders */
class Adopted : public PlainFolder
{
public:

    /** Exception indicating that Adopted cannot be modified  */
    class ModifyException : public Exception { public:
        ModifyException() : Exception("Cannot modify Adopted") {}; };

	virtual ~Adopted(){};

	/**
	 * @param backend backend reference
	 * @param parent reference to parent 
	 */
	Adopted(Backend& backend, Folder& parent);

	virtual void Delete(bool internal = false) override { throw ModifyException(); }

	virtual void Rename(const std::string& name, bool overwrite = false, bool internal = false) override { throw ModifyException(); }

protected:

    /** populate itemMap from the backend */
    virtual void LoadItems() override;
	
private:

	Debug debug;
};

#endif
