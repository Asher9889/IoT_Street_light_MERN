import {
  FaFacebook,
  FaLinkedin,
  FaInstagram,
  FaEnvelope,
} from "react-icons/fa";

export default function Footer() {
  return (
    <footer className="bg-gray-800 text-white h-24 flex items-center mt-10 h-60">
      <div className="max-w-7xl mx-auto px-4 flex flex-col md:flex-row justify-between items-center w-full">
        <div className="mb-2 md:mb-0 text-center md:text-left">
          <h4 className="text-lg font-semibold">
            Street Light Management System
          </h4>
          <p className="text-sm text-gray-400">
            © {new Date().getFullYear()} All rights reserved.
          </p>
        </div>

        <div className="flex space-x-4 mt-2 md:mt-0">
          <a
            href="mailto:yourmail@example.com"
            className="text-gray-400 hover:text-white text-xl"
          >
            <FaEnvelope />
          </a>
          <a
            href="https://facebook.com"
            target="_blank"
            rel="noopener noreferrer"
            className="text-gray-400 hover:text-white text-xl"
          >
            <FaFacebook />
          </a>
          <a
            href="https://linkedin.com"
            target="_blank"
            rel="noopener noreferrer"
            className="text-gray-400 hover:text-white text-xl"
          >
            <FaLinkedin />
          </a>
          <a
            href="https://instagram.com"
            target="_blank"
            rel="noopener noreferrer"
            className="text-gray-400 hover:text-white text-xl"
          >
            <FaInstagram />
          </a>
        </div>
      </div>
    </footer>
  );
}
