// touch all of the getters on an object.  This is used to make properties reactive in mobX
export const strafeObject = (obj) => {
    Object.keys(obj).map(key => obj[key]);
    return obj;
};

