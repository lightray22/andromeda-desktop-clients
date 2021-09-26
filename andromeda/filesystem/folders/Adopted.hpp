
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

    /** Exception indicating that Adopted cannot be deleted  */
    class DeleteException : public Exception { public:
        DeleteException() : Exception("Cannot delete Adopted") {}; };

	virtual ~Adopted(){};

	/**
	 * @param backend backend reference
	 * @param parent reference to parent 
	 */
	Adopted(Backend& backend, Folder& parent);

	virtual void Delete(bool real = false) override { throw DeleteException(); }

protected:

    /** populate itemMap from the backend */
    virtual void LoadItems();
	
private:

	Debug debug;
};

#endif
