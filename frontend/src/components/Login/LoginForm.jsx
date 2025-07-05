import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { HiEye, HiEyeOff, HiX } from "react-icons/hi";
import { FaFacebookF, FaTwitter } from "react-icons/fa";

export default function LoginForm() {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [errors, setErrors] = useState({});

  const nevigatee = useNavigate();

  const togglePassword = () => setShowPassword((prev) => !prev);

  const validate = () => {
    const newErrors = {};
    const emailPattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

    if (!email.trim()) newErrors.email = "Email is required";
    else if (!emailPattern.test(email))
      newErrors.email = "Invalid email format";

    if (!password.trim()) newErrors.password = "Password is required";
    else if (password.length < 6) newErrors.password = "Minimum 6 characters";

    return newErrors;
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    const formErrors = validate();
    if (Object.keys(formErrors).length > 0) setErrors(formErrors);
    else {
      setErrors({});
      console.log("Login Success", { email, password });
    }
  };

  const handleCancel = () => {
    console.log("clicked");
    nevigatee("/");
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-60 flex items-center justify-center z-50 p-4">
      <div className="bg-white w-full max-w-4xl rounded-lg overflow-hidden shadow-lg flex flex-col md:flex-row max-h-[90vh] relative">
        {/* Cancel Icon */}
        <button
          onClick={handleCancel}
          className="absolute top-3 right-3 text-gray-600 hover:text-gray-800"
        >
          <HiX className="h-6 w-6" />
        </button>

        {/* Left Image */}
        <div
          className="md:w-1/2 w-full h-40 md:h-auto bg-cover bg-center"
          style={{ backgroundImage: `url(/slms.jpg)` }}
        ></div>

        {/* Right Login Form */}
        <div className="w-full md:w-1/2 p-8 overflow-y-auto">
          <h2 className="text-2xl font-semibold text-center text-gray-800 mb-6">
            Login to continue
          </h2>

          <form onSubmit={handleSubmit}>
            {/* Email */}
            <div className="mb-4">
              <label className="block text-sm text-gray-700 mb-1">Email</label>
              <input
                type="email"
                placeholder="johndoe@example.com"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
              />
              {errors.email && (
                <p className="text-red-500 text-xs mt-1">{errors.email}</p>
              )}
            </div>

            {/* Password */}
            <div className="mb-4">
              <label className="block text-sm text-gray-700 mb-1">
                Password
              </label>
              <div className="relative">
                <input
                  type={showPassword ? "text" : "password"}
                  placeholder="••••••••"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 pr-10"
                />
                <button
                  type="button"
                  onClick={togglePassword}
                  className="absolute inset-y-0 right-3 flex items-center text-gray-600"
                >
                  {showPassword ? <HiEyeOff /> : <HiEye />}
                </button>
              </div>
              {errors.password && (
                <p className="text-red-500 text-xs mt-1">{errors.password}</p>
              )}
            </div>

            {/* Remember + Forgot */}
            <div className="flex items-center justify-between mb-6 text-sm">
              <label className="flex items-center">
                <input type="checkbox" className="mr-2" />
                Remember me
              </label>
              <a href="#" className="text-blue-500 hover:underline">
                Forgot password?
              </a>
            </div>

            {/* Login Button */}
            <button
              type="submit"
              className="w-full bg-blue-600 text-white py-2 rounded-lg hover:bg-blue-700 transition"
            >
              LOGIN
            </button>
          </form>

          {/* Social logins */}
          <p className="mt-6 text-center text-sm text-gray-600">
            or sign up using
          </p>
          <div className="flex justify-center gap-4 mt-2">
            <button className="p-2 bg-blue-600 text-white rounded-full">
              <FaFacebookF />
            </button>
            <button className="p-2 bg-sky-500 text-white rounded-full">
              <FaTwitter />
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
