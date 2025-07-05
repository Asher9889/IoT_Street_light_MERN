import React from "react";
import { RiArrowDropDownLine } from "react-icons/ri";

const NavBar = ({ navLinks, activeLink, setActiveLink }) => {
  return (
    <nav className="hidden md:flex items-center space-x-10">
      {navLinks.map((link, index) => (
        <div
          key={index}
          onMouseEnter={() => setActiveLink(link)}
          onMouseLeave={() => setActiveLink(null)}
          className="relative"
        >
          <a
            href="#"
            className="flex items-center text-gray-700 hover:text-blue-600 text-sm"
          >
            {link}
            <RiArrowDropDownLine className="h-5 w-5 ml-1" />
          </a>

          {activeLink === link && (
            <div className="absolute top-full left-0 mt-2 bg-white border border-gray-200 rounded shadow-lg w-48 z-10">
              <p className="px-4 py-2 text-gray-700 text-sm">
                Dropdown content for {link}
              </p>
            </div>
          )}
        </div>
      ))}
    </nav>
  );
};

export default NavBar;
