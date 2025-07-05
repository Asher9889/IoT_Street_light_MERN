import { useState } from "react";
import logo from "../../assets/st.png";
import { HiOutlineMenu } from "react-icons/hi";
import { useNavigate } from "react-router-dom";

import NavBar from "../NavBar";

const Header = () => {
  const [showHoverContent, setShowHoverContent] = useState(false);
  const [isMobileMenuOpen, setIsMobileMenuOpen] = useState(false);
  const [activeLink, setActiveLink] = useState(null);

  const negivatee = useNavigate();

  const navLinks = [
    "Product",
    "Resources",
    "Pricing",
    "About us",
    "Contact us",
  ];

  const openLoginForm = () => {
    console.log("Hello");
    negivatee("/register");
  };

  return (
    <header className="bg-white shadow-md h-[14%]">
      <div className="max-w-7xl mx-auto flex items-center justify-between px-6 py-4">
        <div className="flex items-center space-x-3 ml-4">
          <img
            src={logo}
            alt="Logo"
            className="h-10 w-10 object-cover rounded-full"
          />

          <div
            onMouseEnter={() => setShowHoverContent(true)}
            onMouseLeave={() => setShowHoverContent(false)}
            className="relative font-bold text-lg text-gray-800 cursor-pointer"
          >
            LumiNet
            {showHoverContent && (
              <div className="absolute top-8 left-0 bg-gray-200 text-gray-700 px-3 py-1 rounded shadow-md text-sm whitespace-nowrap z-10">
                Smart Control and Monitoring
              </div>
            )}
          </div>
        </div>

        <NavBar
          navLinks={navLinks}
          activeLink={activeLink}
          setActiveLink={setActiveLink}
        />

        <div className="flex items-center space-x-4">
          <button
            onClick={openLoginForm}
            className="bg-blue-500 text-white px-2 py-1 rounded hover:bg-blue-700 text-sm mr-4"
          >
            Login
          </button>

          <button
            className="md:hidden focus:outline-none"
            onClick={() => setIsMobileMenuOpen(!isMobileMenuOpen)}
          >
            <HiOutlineMenu className="h-6 w-6 text-gray-700" />
          </button>
        </div>
      </div>

      {isMobileMenuOpen && (
        <nav className="md:hidden bg-gray-200 px-6 py-4 space-y-3 shadow-md">
          {navLinks.map((link, index) => (
            <a
              href="#"
              key={index}
              className="block text-black hover:text-blue-600 text-sm"
            >
              {link}
            </a>
          ))}
        </nav>
      )}
    </header>
  );
};

export default Header;
