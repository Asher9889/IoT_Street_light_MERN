import React from "react";
// import heroImage from "../assets/st.png";
import heroImage from "../../assets/st.png";
import { useNavigate } from "react-router-dom";
const Main = () => {
  const negivatee = useNavigate();
  const openLoginForm = () => {
    console.log("Hello");
    negivatee("/register");
  };
  return (
    <section className="relative w-full h-screen">
      {/* <img
        src={heroImage}
        alt="Hero"
        className="absolute inset-0 w-full h-full object-cover"
      /> */}

      <div
        className="absolute inset-0 flex flex-col justify-center items-center text-center px-4 bg-black bg-opacity-40"
        style={{ backgroundColor: "rgba(249, 249, 251, 0.8)" }}
      >
        <h1 className="text-3xl md:text-5xl font-bold drop-shadow-lg text-[rgba(0,0,0,0.6)]">
          <span className="">Smart </span>
          <span
            style={{
              background:
                "linear-gradient(90deg, #2253FF 18.57%, #2253FF 42.47%, #FF5622 57.68%, #FF5622 79.75%)",
              WebkitBackgroundClip: "text",
              WebkitTextFillColor: "transparent",
              backgroundClip: "text",
              color: "transparent",
            }}
          >
            Lighting Management
          </span>
          <span> System</span>
        </h1>

        <p className="mt-6 text-black text-3xl md:text-lg max-w-2xl font-semibold">
          Step into the next era of illumination with AI-powered smart lighting
          control.
        </p>

        <p className="text-black text-sm md:text-lg max-w-xl">
          Monitor and control street lights efficiently with real-time data and
          automation.
        </p>
        {/* <button className="mt-6 bg-blue-400 text-black px-6 py-3 rounded-3xl w-32 py-1 hover:bg-blue-700 transition text-sm md:text-base">
          Log in
        </button> */}

        <button
          onClick={openLoginForm}
          className=" h-10 w-32
            relative inline-flex items-center justify-center
            px-6 py-0.5 rounded font-semibold cursor-pointer
            text-gray-800 shadow-[0_4px_10px_rgba(0,0,0,0.2)]
            overflow-hidden transition-all duration-200 active:scale-95
            hover:bg-blue-200 hover:text-purple-900 mt-4"
        >
          <span className="z-10">Login</span>
          <span className="absolute inset-0 flex items-center justify-center z-0">
            <span
              className="
                w-40 h-40 rounded-full opacity-50 blur-[20px]
                bg-gradient-to-r from-pink-600 via-purple-500 to-cyan-400
                animate-spin-slow transition-all duration-400
                hover:w-32 hover:h-32"
            ></span>
          </span>
        </button>
      </div>
    </section>
  );
};

export default Main;
