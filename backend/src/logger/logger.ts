import winston from "winston";
import DailyRotateFile from "winston-daily-rotate-file";
import path from "path";

// Helper to format timestamps cleanly
const timestampFormat = winston.format.timestamp({ format: "YYYY-MM-DD HH:mm:ss" });

// Log format for console: human-friendly
const consoleFormat = winston.format.combine(
  winston.format.colorize(),
  timestampFormat,
  winston.format.printf(({ level, message, timestamp, stack }) => {
    return stack
      ? `[${timestamp}] ${level}: ${message}\n${stack}`
      : `[${timestamp}] ${level}: ${message}`;
  })
);

// Log format for files: structured JSON
const fileFormat = winston.format.combine(timestampFormat, winston.format.json());

// Log directory
const logDir = path.resolve("logs");

console.log("Logger initialized", logDir)

// Create the base logger
const logger = winston.createLogger({
  level: process.env.LOG_LEVEL || "info",
  format: fileFormat,
  defaultMeta: { service: "gateway-backend" },
  transports: [
    // ✅ Rotate error logs daily
    new DailyRotateFile({
      dirname: logDir,
      filename: "error-%DATE%.log",
      datePattern: "YYYY-MM-DD",
      level: "error",
      maxFiles: "14d", // keep 14 days
      zippedArchive: true,
    }),
    // ✅ Rotate all logs daily
    new DailyRotateFile({
      dirname: logDir,
      filename: "combined-%DATE%.log",
      datePattern: "YYYY-MM-DD",
      maxFiles: "14d",
      zippedArchive: true,
    }),
  ],
});

// ✅ In dev, also log to console in color
if (process.env.NODE_ENV !== "production") {
  logger.add(new winston.transports.Console({ format: consoleFormat }));
}



export default logger;
